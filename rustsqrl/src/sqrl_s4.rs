use std::fmt;
use std::str;
use extfmt::Hexlify;

use byte::*;
use byte::ctx::{Str, Bytes, Endian};
//use byte::ctx::*;


#[derive(Debug, Default)]
pub struct SqrlS4Identity<'a> {
    pub header:                                 &'a str,
    pub type1_length:                           u16,
    pub type1_type:                             u16,
    pub type1_pt_length:                        u16,
    pub type1_aes_gcm_iv:                       &'a[u8],
    pub type1_scrypt_random_salt:               &'a[u8],
    pub type1_scrypt_log_n_factor:              u8,
    pub type1_scrypt_iteration_count:           u32,
    pub type1_option_flags:                     u16,
    pub type1_hint_length:                      u8,
    pub type1_pw_verify_sec:                    u8,
    pub type1_idle_timeout_min:                 u16,
    pub type1_encrypted_identity_master_key:    &'a[u8],
    pub type1_encrypted_identity_lock_key:      &'a[u8],
    pub type1_verification_tag:                 &'a[u8],
    pub type2_length:                           u16,
    pub type2_type:                             u16,
    pub type2_scrypt_random_salt:               &'a[u8],
    pub type2_scrypt_log_n_factor:              u8,
    pub type2_scrypt_iteration_count:           u32,
    pub type2_encrypted_identity_unlock_key:    &'a[u8],
    pub type2_verification_tag:                 &'a[u8],
}


impl<'a> fmt::Display for SqrlS4Identity<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Type1: user access password protected data\n")?;
        write!(f, "  header:                          {}\n", self.header)?;
        write!(f, "  length:                          {}\n", self.type1_length)?;
        write!(f, "  type:                            {}\n", self.type1_type)?;
        write!(f, "  pt_length:                       {}\n", self.type1_pt_length)?;
        write!(f, "  aes_gcm_iv:                      {}\n", Hexlify(&self.type1_aes_gcm_iv))?;
        write!(f, "  scrypt_random_salt:              {}\n", Hexlify(&self.type1_scrypt_random_salt))?;
        write!(f, "  scrypt_log_n_factor:             {}\n", self.type1_scrypt_log_n_factor)?;
        write!(f, "  scrypt_iteration_count:          {}\n", self.type1_scrypt_iteration_count)?;
        write!(f, "  option_flags:                    {}\n", self.type1_option_flags)?;
        write!(f, "  hint_length:                     {}\n", self.type1_hint_length)?;
        write!(f, "  pw_verify_sec:                   {}\n", self.type1_pw_verify_sec)?;
        write!(f, "  idle_timeout_min:                {}\n", self.type1_idle_timeout_min)?;
        write!(f, "  encrypted_identity_master_key:   {}\n", Hexlify(&self.type1_encrypted_identity_master_key))?;
        write!(f, "  encrypted_identity_lock_key:     {}\n", Hexlify(&self.type1_encrypted_identity_lock_key))?;
        write!(f, "  verification_tag:                {}\n", Hexlify(&self.type1_verification_tag))?;

        write!(f, "\nType2: rescue code data\n")?;
        write!(f, "  length:                          {}\n", self.type2_length)?;
        write!(f, "  type:                            {}\n", self.type2_type)?;
        write!(f, "  scrypt_random_salt:              {}\n", Hexlify(&self.type2_scrypt_random_salt))?;
        write!(f, "  scrypt_log_n_factor:             {}\n", self.type2_scrypt_log_n_factor)?;
        write!(f, "  scrypt_iteration_count:          {}\n", self.type2_scrypt_iteration_count)?;
        write!(f, "  encrypted_identity_unlock_key:   {}\n", Hexlify(&self.type2_encrypted_identity_unlock_key))?;
        write!(f, "  verification_tag:                {}\n", Hexlify(&self.type2_verification_tag))?;

        Ok(())
    }
}


impl<'a> TryRead<'a, Endian> for SqrlS4Identity<'a> {
    fn try_read(sqrlbinary: &'a [u8], endian: Endian) -> Result<(Self, usize)> {
        assert!(sqrlbinary.len() >= 206);
        assert!(sqrlbinary.len() == 206, "Identity with pidk data not supported"); // TODO: implement

        let offset = &mut 0;
        let identity = SqrlS4Identity {
                header:                                 sqrlbinary.read_with(offset, Str::Len(8))?,
                type1_length:                           sqrlbinary.read_with(offset, endian)?,
                type1_type:                             sqrlbinary.read_with(offset, endian)?,
                type1_pt_length:                        sqrlbinary.read_with(offset, endian)?,
                type1_aes_gcm_iv:                       sqrlbinary.read_with(offset, Bytes::Len(12))?,
                type1_scrypt_random_salt:               sqrlbinary.read_with(offset, Bytes::Len(16))?,
                type1_scrypt_log_n_factor:              sqrlbinary.read_with(offset, endian)?,
                type1_scrypt_iteration_count:           sqrlbinary.read_with(offset, endian)?,
                type1_option_flags:                     sqrlbinary.read_with(offset, endian)?,
                type1_hint_length:                      sqrlbinary.read_with(offset, endian)?,
                type1_pw_verify_sec:                    sqrlbinary.read_with(offset, endian)?,
                type1_idle_timeout_min:                 sqrlbinary.read_with(offset, endian)?,
                type1_encrypted_identity_master_key:    sqrlbinary.read_with(offset, Bytes::Len(32))?,
                type1_encrypted_identity_lock_key:      sqrlbinary.read_with(offset, Bytes::Len(32))?,
                type1_verification_tag:                 sqrlbinary.read_with(offset, Bytes::Len(16))?,
                type2_length:                           sqrlbinary.read_with(offset, endian)?,
                type2_type:                             sqrlbinary.read_with(offset, endian)?,
                type2_scrypt_random_salt:               sqrlbinary.read_with(offset, Bytes::Len(16))?,
                type2_scrypt_log_n_factor:              sqrlbinary.read_with(offset, endian)?,
                type2_scrypt_iteration_count:           sqrlbinary.read_with(offset, endian)?,
                type2_encrypted_identity_unlock_key:    sqrlbinary.read_with(offset, Bytes::Len(32))?,
                type2_verification_tag:                 sqrlbinary.read_with(offset, Bytes::Len(16))?,
        };

        assert!(identity.header == "sqrldata");
        assert!(identity.type1_length == 125);
        assert!(identity.type1_scrypt_iteration_count > 0);
        assert!(identity.type2_length == 73);

        Ok((identity, *offset))
    }
}


impl<'a> SqrlS4Identity<'a> {
    pub fn from_binary(sqrlbinary: &[u8]) -> SqrlS4Identity {
        // TODO: Consider returning Result<SqrlS4Identity>
        sqrlbinary.read_with::<SqrlS4Identity>(&mut 0, LE).unwrap()
    }
}


#[cfg(test)]
mod tests {
    //mod sqrl_s4;
    //use crate::sqrl_s4::SqrlS4Identity;
    use super::*;

    #[test]
    fn test_from_binary_valid() {
        let sqrlbinary = b"sqrldata}\x00\x01\x00-\x00\"wQ\x122\x0e\xb5\x891\xfep\x97\xef\xf2e]\xf6\x0fg\x07\x8c_\xda\xd4\xe0Z\xe0\xb8\t\x96\x00\x00\x00\xf3\x01\x04\x05\x0f\x00\x023\x88\xcd\xa0\xd7WN\xf7\x8a\xd19\xf8\x1c]\x13\x87\x06\xc6\xe8\xf8\xb08\xf6\x14\xd9m\x9e\xf6|\x94\xa4\x1fF\xab}\x0e\xd3\xbf\xa3r\xa3^\xb4\xfb\xcc\xe7\x8cQ\x8d\x8dyRl\x05\xf1\x19|\x90\x03\x06\t\xe0\xb3\x85H\x8c\xe0\xa6\x0fQm\xf6\x94q6-\xee\xe0\xe9I\x00\x02\x00\xea\xde\x04q\xa1\xfaO\x8f\x1c\xf5e\xea\xb3)-^\t\xa5\x00\x00\x00\xf9o$\"\x9e\x91\xa6\xa9k\xde\xe2z^&j\xa6\x15\xb5\x04\xf4P\x01e\xcc\xfa\xa8V\xd7\xf4\x94L\xea\xea\xdd><\xcbC\xc5+\xeb\xaf\x18\x88\xf9\xa6\xd4\xce";
        let identity = SqrlS4Identity::from_binary(sqrlbinary);
        //println!("{}", identity);
        assert_eq!(identity.type1_length,                           125);
        assert_eq!(identity.type1_pt_length,                        45);
        assert_eq!(identity.type1_aes_gcm_iv,                       [0x22,0x77,0x51,0x12,0x32,0x0e,0xb5,0x89,0x31,0xfe,0x70,0x97]);
        assert_eq!(identity.type1_scrypt_random_salt,               [0xef,0xf2,0x65,0x5d,0xf6,0x0f,0x67,0x07,0x8c,0x5f,0xda,0xd4,0xe0,0x5a,0xe0,0xb8]);
        assert_eq!(identity.type1_scrypt_log_n_factor,              9);
        assert_eq!(identity.type1_scrypt_iteration_count,           150);
        assert_eq!(identity.type1_option_flags,                     499);
        assert_eq!(identity.type1_hint_length,                      4);
        assert_eq!(identity.type1_pw_verify_sec,                    5);
        assert_eq!(identity.type1_idle_timeout_min,                 15);
        assert_eq!(identity.type1_encrypted_identity_master_key,    [0x02,0x33,0x88,0xcd,0xa0,0xd7,0x57,0x4e,0xf7,0x8a,0xd1,0x39,0xf8,0x1c,0x5d,0x13,0x87,0x06,0xc6,0xe8,0xf8,0xb0,0x38,0xf6,0x14,0xd9,0x6d,0x9e,0xf6,0x7c,0x94,0xa4]);
        assert_eq!(identity.type1_encrypted_identity_lock_key,      [0x1f,0x46,0xab,0x7d,0x0e,0xd3,0xbf,0xa3,0x72,0xa3,0x5e,0xb4,0xfb,0xcc,0xe7,0x8c,0x51,0x8d,0x8d,0x79,0x52,0x6c,0x05,0xf1,0x19,0x7c,0x90,0x03,0x06,0x09,0xe0,0xb3]);
        assert_eq!(identity.type1_verification_tag,                 [0x85,0x48,0x8c,0xe0,0xa6,0x0f,0x51,0x6d,0xf6,0x94,0x71,0x36,0x2d,0xee,0xe0,0xe9]);

        assert_eq!(identity.type2_length,                           73);
        assert_eq!(identity.type2_type,                             2);
        assert_eq!(identity.type2_scrypt_random_salt,               [0xea,0xde,0x04,0x71,0xa1,0xfa,0x4f,0x8f,0x1c,0xf5,0x65,0xea,0xb3,0x29,0x2d,0x5e]);
        assert_eq!(identity.type2_scrypt_log_n_factor,              9);
        assert_eq!(identity.type2_scrypt_iteration_count,           165);
        assert_eq!(identity.type2_encrypted_identity_unlock_key,    [0xf9,0x6f,0x24,0x22,0x9e,0x91,0xa6,0xa9,0x6b,0xde,0xe2,0x7a,0x5e,0x26,0x6a,0xa6,0x15,0xb5,0x04,0xf4,0x50,0x01,0x65,0xcc,0xfa,0xa8,0x56,0xd7,0xf4,0x94,0x4c,0xea]);
        assert_eq!(identity.type2_verification_tag,                 [0xea,0xdd,0x3e,0x3c,0xcb,0x43,0xc5,0x2b,0xeb,0xaf,0x18,0x88,0xf9,0xa6,0xd4,0xce]);
    }

    #[test]
    #[should_panic]
    fn test_from_binary_too_short() {
        let sqrlbinary = b"sqrldata}\x00\x01\x00-\x00\"wQ\x122";
        let identity = SqrlS4Identity::from_binary(sqrlbinary);
        println!("{}", identity);
    }

    #[test]
    #[should_panic]
    fn test_from_binary_invalid_header() {
        let sqrlbinary = b"invalid_}\x00\x01\x00-\x00\"wQ\x122\x0e\xb5\x891\xfep\x97\xef\xf2e]\xf6\x0fg\x07\x8c_\xda\xd4\xe0Z\xe0\xb8\t\x96\x00\x00\x00\xf3\x01\x04\x05\x0f\x00\x023\x88\xcd\xa0\xd7WN\xf7\x8a\xd19\xf8\x1c]\x13\x87\x06\xc6\xe8\xf8\xb08\xf6\x14\xd9m\x9e\xf6|\x94\xa4\x1fF\xab}\x0e\xd3\xbf\xa3r\xa3^\xb4\xfb\xcc\xe7\x8cQ\x8d\x8dyRl\x05\xf1\x19|\x90\x03\x06\t\xe0\xb3\x85H\x8c\xe0\xa6\x0fQm\xf6\x94q6-\xee\xe0\xe9I\x00\x02\x00\xea\xde\x04q\xa1\xfaO\x8f\x1c\xf5e\xea\xb3)-^\t\xa5\x00\x00\x00\xf9o$\"\x9e\x91\xa6\xa9k\xde\xe2z^&j\xa6\x15\xb5\x04\xf4P\x01e\xcc\xfa\xa8V\xd7\xf4\x94L\xea\xea\xdd><\xcbC\xc5+\xeb\xaf\x18\x88\xf9\xa6\xd4\xce";
        let identity = SqrlS4Identity::from_binary(sqrlbinary);
        println!("{}", identity);
    }
}
