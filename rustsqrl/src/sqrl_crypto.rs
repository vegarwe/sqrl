use extfmt::Hexlify;
use scrypt::{ScryptParams, scrypt};

pub fn en_scrypt(password: &[u8], salt: &[u8], mut n: u32, iteration_count: u32) -> [u8; 32] {
    let mut log_n = 0;
    loop { // Find integer log_2 of n
        n >>= 1;
        if n == 0 { break; }
        log_n += 1;
    };
    let r = 256;
    let p = 1;      // disable parallelism

    let params = ScryptParams::new(log_n, r, p).unwrap();
    let mut out = [0u8; 32];
    let mut tmp = [0u8; 32];
    let mut key = [0u8; 32];

    for i in 0..iteration_count {
        if i == 0 {
            scrypt(password, salt, &params, &mut out).unwrap();
        } else {
            scrypt(password, &tmp, &params, &mut out).unwrap();
        }

        for i in 0..tmp.len() {
            key[i] ^= out[i];
            tmp[i] = out[i];
        }
    }

    key
}


#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_simple() {
        assert_eq!(en_scrypt(b"", b"", 64,  1), [0x81,0xdb,0xe0,0x3c,0x5f,0xd9,0x46,0x37,0x5b,0x44,0xfc,0x9c,0xcd,0x9b,0x98,0x47,0xb5,0x82,0x01,0x3f,0xfe,0x09,0xab,0x7f,0x01,0x50,0xec,0xcd,0xe0,0x28,0xd6,0x77]);
        assert_eq!(en_scrypt(b"", b"", 64,  2), [0x16,0xe3,0xc9,0xdf,0xd1,0x86,0x12,0x9c,0xff,0x70,0x2d,0x1b,0xed,0x4e,0xe7,0xdd,0x0c,0x1b,0x27,0x41,0x7c,0x48,0xc2,0xc7,0x2c,0x27,0x94,0x17,0x01,0xf3,0xa4,0x59]);
        assert_eq!(en_scrypt(b"", b"", 64, 10), [0x2e,0x07,0xc0,0x00,0x6b,0x4b,0xfa,0x7f,0x18,0x51,0x0a,0x2e,0x4f,0xd9,0x75,0x1c,0x34,0x2c,0x12,0xfa,0x0c,0x06,0xfa,0xc8,0xc1,0x47,0x5d,0x9e,0x8a,0x2f,0x8c,0x76]);

        //print(binascii.hexlify(EnScrypt('', '', 64, 1)))
        // b'81dbe03c5fd946375b44fc9ccd9b9847b582013ffe09ab7f0150eccde028d677'
        //print(binascii.hexlify(EnScrypt('', '', 64, 2)))
        // b'16e3c9dfd186129cff702d1bed4ee7dd0c1b27417c48c2c72c27941701f3a459'
        //print(binascii.hexlify(EnScrypt('', '', 64, 10)))
        // b'2e07c0006b4bfa7f18510a2e4fd9751c342c12fa0c06fac8c1475d9e8a2f8c76'
        //
        //print(binascii.hexlify(EnScrypt('', '', 512, 1)))
        // b'a8ea62a6e1bfd20e4275011595307aa302645c1801600ef5cd79bf9d884d911c'
        //print(binascii.hexlify(EnScrypt('', '', 512, 100)))
        // b'45a42a01709a0012a37b7b6874cf16623543409d19e7740ed96741d2e99aab67'
        //print(binascii.hexlify(EnScrypt('', '', 512, 1000)))
        // b'3f671adf47d2b1744b1bf9b50248cc71f2a58e8d2b43c76edb1d2a2c200907f5'
        //print(binascii.hexlify(EnScrypt('password', '', 512, 123)))
        // b'129d96d1e735618517259416a605be7094c2856a53c14ef7d4e4ba8e4ea36aeb'
    }
}
