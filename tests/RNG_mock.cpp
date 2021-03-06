/*
 * Copyright (C) 2015 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "RNG.h"
//#include "NoiseSource.h"
#include "Crypto.h"
#include "utility/ProgMemUtil.h"
#include <string.h>

/**
 * \class RNGClass RNG.h <RNG.h>
 * \brief Pseudo random number generator suitable for cryptography.
 *
 * Random number generators must be seeded properly before they can
 * be used or an adversary may be able to predict the random output.
 * Seed data may be:
 *
 * \li Device-specific, for example serial numbers or MAC addresses.
 * \li Application-specific, unique to the application.  The tag that is
 * passed to begin() is an example of an application-specific value.
 * \li Noise-based, generated by a hardware random number generator
 * that provides unpredictable values from a noise source.
 *
 * The following example demonstrates how to initialise the random
 * number generator:
 *
 * \code
 * #include <SPI.h>
 * #include <Ethernet.h>
 * #include <Crypto.h>
 * #include <RNG.h>
 * #include <TransistorNoiseSource.h>
 *
 * // Noise source to seed the random number generator.
 * TransistorNoiseSource noise(A1);
 *
 * // MAC address for Ethernet communication.
 * byte mac_address[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
 *
 * void setup() {
 *     // Initialize the Ethernet shield.
 *     Ethernet.begin(mac_address);
 *
 *     // Initialize the random number generator with the application tag
 *     // "MyApp 1.0" and load the previous seed from EEPROM.
 *     RNG.begin("MyApp 1.0");
 *
 *     // Stir in the Ethernet MAC address.
 *     RNG.stir(mac_address, sizeof(mac_address));
 *
 *     // Add the noise source to the list of sources known to RNG.
 *     RNG.addNoiseSource(noise);
 *
 *     // ...
 * }
 * \endcode
 *
 * The application should regularly call loop() to stir in new data
 * from the registered noise sources and to periodically save the seed:
 *
 * \code
 * void loop() {
 *     // ...
 *
 *     // Perform regular housekeeping on the random number generator.
 *     RNG.loop();
 *
 *     // ...
 * }
 * \endcode
 *
 * The loop() function will automatically save the random number seed on a
 * regular basis to the last SEED_SIZE bytes of EEPROM memory.  By default
 * the seed is saved every hour but this can be changed using setAutoSaveTime().
 *
 * Keep in mind that saving too often may cause the EEPROM to wear out quicker.
 * It is wise to limit saving to once an hour or once a day depending
 * upon how long you intend to field the device before replacing it.
 * For example, an EEPROM rated for 100k erase/write cycles will last about
 * 69 days saving once a minute or 11 years saving once an hour.
 *
 * The application can still elect to call save() at any time if wants.
 * For example, if the application can detect power loss or shutdown
 * conditions programmatically, then it may make sense to force a save()
 * of the seed upon shutdown.
 *
 * The Arduino Due does not have EEPROM so RNG saves the seed into
 * the last page of system flash memory instead.  The RNG class will also
 * mix in data from the CPU's built-in True Random Number Generator (TRNG).
 * Assuming that the CPU's TRNG is trustworthy, this should be sufficient
 * to properly seed the random number generator.  It is recommended to
 * also mix in data from other noise sources just in case the CPU's TRNG
 * is not trustworthy.
 *
 * \sa NoiseSource
 */

/**
 * \brief Global random number generator instance.
 *
 * \sa RNGClass
 */
RNGClass RNG;

/**
 * \var RNGClass::SEED_SIZE
 * \brief Size of a saved random number seed in EEPROM space.
 *
 * The seed is saved into the last SEED_SIZE bytes of EEPROM memory.
 * The address is dependent upon the size of EEPROM fitted in the device.
 */

// Number of ChaCha hash rounds to use for random number generation.
#define RNG_ROUNDS          20

// Force a rekey after this many blocks of random data.
#define RNG_REKEY_BLOCKS    16

// Maximum entropy credit that can be contained in the pool.
#define RNG_MAX_CREDITS     384u

/** @cond */

// Imported from Crypto.cpp.
extern uint8_t crypto_crc8(uint8_t tag, const void *data, unsigned size);

// Tag for 256-bit ChaCha20 keys.  This will always appear in the
// first 16 bytes of the block.  The remaining 48 bytes are the seed.
static const char tagRNG[16] PROGMEM = {
    'e', 'x', 'p', 'a', 'n', 'd', ' ', '3',
    '2', '-', 'b', 'y', 't', 'e', ' ', 'k'
};

// Initialization seed.  This is the ChaCha20 output of hashing
// "expand 32-byte k" followed by 48 bytes set to the numbers 1 to 48.
// The ChaCha20 output block is then truncated to the first 48 bytes.
//
// This value is intended to start the RNG in a semi-chaotic state if
// we don't have a previously saved seed in EEPROM.
static const uint8_t initRNG[48] PROGMEM = {
    0xB0, 0x2A, 0xAE, 0x7D, 0xEE, 0xCB, 0xBB, 0xB1,
    0xFC, 0x03, 0x6F, 0xDD, 0xDC, 0x7D, 0x76, 0x67,
    0x0C, 0xE8, 0x1F, 0x0D, 0xA3, 0xA0, 0xAA, 0x1E,
    0xB0, 0xBD, 0x72, 0x6B, 0x2B, 0x4C, 0x8A, 0x7E,
    0x34, 0xFC, 0x37, 0x60, 0xF4, 0x1E, 0x22, 0xA0,
    0x0B, 0xFB, 0x18, 0x84, 0x60, 0xA5, 0x77, 0x72
};


/** @endcond */

/**
 * \brief Constructs a new random number generator instance.
 *
 * This constructor must be followed by a call to begin() to
 * properly initialize the random number generator.
 *
 * \sa begin()
 */
RNGClass::RNGClass()
    : credits(0)
    , firstSave(1)
    , initialized(0)
    , trngPending(0)
    , timer(0)
    , timeout(3600000UL)    // 1 hour in milliseconds
    , count(0)
    , trngPosn(0)
{
}

/**
 * \brief Destroys this random number generator instance.
 */
RNGClass::~RNGClass()
{
    clean(block);
    clean(stream);
}

/**
 * \brief Initializes the random number generator.
 *
 * \param tag A string that is stirred into the random pool at startup;
 * usually this should be a value that is unique to the application and
 * version such as "MyApp 1.0" so that different applications do not
 * generate the same sequence of values upon first boot.
 *
 * This function should be followed by calls to addNoiseSource() to
 * register the application's noise sources.
 *
 * \sa addNoiseSource(), stir(), save()
 */
void RNGClass::begin(const char *tag)
{
    initialized = 1;
}

/**
 * \brief Adds a noise source to the random number generator.
 *
 * \param source The noise source to add, which will be polled regularly
 * by loop() to accumulate noise-based entropy from the source.
 *
 * RNG supports a maximum of four noise sources.  If the application needs
 * more than that then the application must poll the noise sources itself by
 * calling NoiseSource::stir() directly.
 *
 * \sa loop(), begin()
 */
void RNGClass::addNoiseSource(NoiseSource &source)
{
}

/**
 * \brief Sets the amount of time between automatic seed saves.
 *
 * \param minutes The number of minutes between automatic seed saves.
 *
 * The default time between automatic seed saves is 1 hour.
 *
 * This function is intended to help with EEPROM wear by slowing down how
 * often seed data is saved as noise is stirred into the random pool.
 * The exact period to use depends upon how long you intend to field
 * the device before replacing it.  For example, an EEPROM rated for
 * 100k erase/write cycles will last about 69 days saving once a minute
 * or 11 years saving once an hour.
 *
 * \sa save(), stir()
 */
void RNGClass::setAutoSaveTime(uint16_t minutes)
{
}

/**
 * \brief Generates random bytes into a caller-supplied buffer.
 *
 * \param data Points to the buffer to fill with random bytes.
 * \param len Number of bytes to generate.
 *
 * Calling this function will decrease the amount of entropy in the
 * random number pool by \a len * 8 bits.  If there isn't enough
 * entropy, then this function will still return \a len bytes of
 * random data generated from what entropy it does have.
 *
 * If the application requires a specific amount of entropy before
 * generating important values, the available() function can be
 * polled to determine when sufficient entropy is available.
 *
 * \sa available(), stir()
 */
void RNGClass::rand(uint8_t *data, size_t len)
{
}

/**
 * \brief Determine if there is sufficient entropy available for a
 * specific request size.
 *
 * \param len The number of bytes of random data that will be requested
 * via a call to rand().
 * \return Returns true if there is at least \a len * 8 bits of entropy
 * in the random number pool, or false if not.
 *
 * This function can be used by the application to wait for sufficient
 * entropy to become available from the system's noise sources before
 * generating important values.  For example:
 *
 * \code
 * bool haveKey = false;
 * byte key[32];
 *
 * void loop() {
 *     ...
 *
 *     if (!haveKey && RNG.available(sizeof(key))) {
 *         RNG.rand(key, sizeof(key));
 *         haveKey = true;
 *     }
 *
 *     ...
 * }
 * \endcode
 *
 * If \a len is larger than the maximum number of entropy credits supported
 * by the random number pool (384 bits, 48 bytes), then the maximum will be
 * used instead.  For example, asking if 512 bits (64 bytes) are available
 * will return true if in reality only 384 bits are available.  If this is a
 * problem for the application's security requirements, then large requests
 * for random data should be broken up into smaller chunks with the
 * application waiting for the entropy pool to refill between chunks.
 *
 * \sa rand()
 */
bool RNGClass::available(size_t len) const
{
    return true;
}

/**
 * \brief Stirs additional entropy data into the random pool.
 *
 * \param data Points to the additional data to be stirred in.
 * \param len Number of bytes to be stirred in.
 * \param credit The number of bits of entropy to credit for the
 * data that is stirred in.  Note that this is bits, not bytes.
 *
 * The maximum credit allowed is \a len * 8 bits, indicating that
 * every bit in the input \a data is good and random.  Practical noise
 * sources are rarely that good, so \a credit will usually be smaller.
 * For example, to credit 2 bits of entropy per byte, the function
 * would be called as follows:
 *
 * \code
 * RNG.stir(noise_data, noise_bytes, noise_bytes * 2);
 * \endcode
 *
 * If \a credit is zero, then the \a data will be stirred in but no
 * entropy credit is given.  This is useful for static values like
 * serial numbers and MAC addresses that are different between
 * devices but highly predictable.
 *
 * \sa loop()
 */
void RNGClass::stir(const uint8_t *data, size_t len, unsigned int credit)
{
}

/**
 * \brief Saves the random seed to EEPROM.
 *
 * During system startup, noise sources typically won't have accumulated
 * much entropy.  But startup is usually the time when the system most
 * needs to generate random data for session keys, IV's, and the like.
 *
 * The purpose of this function is to pass some of the accumulated entropy
 * from one session to the next after a loss of power.  Thus, once the system
 * has been running for a while it will get progressively better at generating
 * random values and the accumulated entropy will not be completely lost.
 *
 * Normally it isn't necessary to call save() directly.  The loop() function
 * will automatically save the seed on a periodic basis (default of 1 hour).
 *
 * The seed that is saved is generated in such a way that it cannot be used
 * to predict random values that were generated previously or subsequently
 * in the current session.  So a compromise of the EEPROM contents of a
 * captured device should not result in compromise of random values
 * that have already been generated.  However, if power is lost and the
 * system restarted, then there will be a short period of time where the
 * random state will be predictable from the seed.  For this reason it is
 * very important to stir() in new noise data at startup.
 *
 * \sa loop(), stir()
 */
void RNGClass::save()
{
}

/**
 * \brief Run periodic housekeeping tasks on the random number generator.
 *
 * This function must be called on a regular basis from the application's
 * main "loop()" function.
 */
void RNGClass::loop()
{
}

/**
 * \brief Destroys the data in the random number pool and the saved seed
 * in EEPROM.
 *
 * This function attempts to throw away any data that could theoretically be
 * used to predict previous and future outputs of the random number generator
 * if the device is captured, sold, or otherwise compromised.
 *
 * After this function is called, begin() must be called again to
 * re-initialize the random number generator.
 *
 * \note The rand() and save() functions take some care to manage the
 * random number pool in a way that makes prediction of past outputs from a
 * captured state very difficult.  Future outputs may be predictable if
 * noise or other high-entropy data is not mixed in with stir() on a
 * regular basis.
 *
 * \sa begin()
 */
void RNGClass::destroy()
{
    clean(block);
    clean(stream);
    initialized = 0;
}

/**
 * \brief Rekeys the random number generator.
 */
void RNGClass::rekey()
{
}

/**
 * \brief Mix in fresh data from the TRNG when rand() is called.
 */
void RNGClass::mixTRNG()
{
}
