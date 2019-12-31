use std::fmt;
use std::str;
use extfmt::Hexlify;

use byte::*;
//use byte::ctx::*;

#[derive(Debug, Default)]
pub struct SqrlS4Identity {
    header:                                 [u8;  8],
    type1_length:                           u16,
    type1_type:                             u16,
    type1_pt_length:                        u16,
    type1_aes_gcm_iv:                       [u8; 12],
    type1_scrypt_random_salt:               [u8; 16],
    type1_scrypt_log_n_factor:              u8,
    type1_scrypt_iteration_count:           u32,
    type1_option_flags:                     u16,
    type1_hint_length:                      u8,
    type1_pw_verify_sec:                    u8,
    type1_idle_timeout_min:                 u16,
    type1_encrypted_identity_master_key:    [u8; 32],
    type1_encrypted_identity_lock_key:      [u8; 32],
    type1_verification_tag:                 [u8; 16],
    type2_length:                           u16,
    type2_type:                             u16,
    type2_scrypt_random_salt:               [u8; 16],
    type2_scrypt_log_n_factor:              u8,
    type2_scrypt_iteration_count:           u32,
    type2_encrypted_identity_unlock_key:    [u8; 32],
    type2_verification_tag:                 [u8; 16],
}


impl fmt::Display for SqrlS4Identity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if let Err(_) = f.write_str("Type1: user access password protected data\n")                                                 { return Ok(()); }
        if let Ok(s) = str::from_utf8(&self.header) { if let Err(_) = write!(f, "  magic:                            {}\n", s)      { return Ok(()); }}
        if let Err(_) = write!(f, "  length:                           {}\n", self.type1_length)                                    { return Ok(()); }
        if let Err(_) = write!(f, "  type:                             {}\n", self.type1_type)                                      { return Ok(()); }
        if let Err(_) = write!(f, "  pt_length:                        {}\n", self.type1_pt_length)                                 { return Ok(()); }
        if let Err(_) = write!(f, "  aes_gcm_iv:                       {}\n", Hexlify(&self.type1_aes_gcm_iv))                      { return Ok(()); }
        if let Err(_) = write!(f, "  scrypt_random_salt:               {}\n", Hexlify(&self.type1_scrypt_random_salt))              { return Ok(()); }
        if let Err(_) = write!(f, "  scrypt_log_n_factor:              {}\n", self.type1_scrypt_log_n_factor)                       { return Ok(()); }
        if let Err(_) = write!(f, "  scrypt_iteration_count:           {}\n", self.type1_scrypt_iteration_count)                    { return Ok(()); }
        if let Err(_) = write!(f, "  option_flags:                     {}\n", self.type1_option_flags)                              { return Ok(()); }
        if let Err(_) = write!(f, "  hint_length:                      {}\n", self.type1_hint_length)                               { return Ok(()); }
        if let Err(_) = write!(f, "  pw_verify_sec:                    {}\n", self.type1_pw_verify_sec)                             { return Ok(()); }
        if let Err(_) = write!(f, "  idle_timeout_min:                 {}\n", self.type1_idle_timeout_min)                          { return Ok(()); }
        if let Err(_) = write!(f, "  encrypted_identity_master_key:    {}\n", Hexlify(&self.type1_encrypted_identity_master_key))   { return Ok(()); }
        if let Err(_) = write!(f, "  encrypted_identity_lock_key:      {}\n", Hexlify(&self.type1_encrypted_identity_lock_key))     { return Ok(()); }
        if let Err(_) = write!(f, "  verification_tag:                 {}\n", Hexlify(&self.type1_verification_tag))                { return Ok(()); }

        if let Err(_) = f.write_str("\nType2: rescue code data\n")                                                                  { return Ok(()); }
        if let Err(_) = write!(f, "  length:                           {}\n", self.type2_length)                                    { return Ok(()); }
        if let Err(_) = write!(f, "  type:                             {}\n", self.type2_type)                                      { return Ok(()); }
        if let Err(_) = write!(f, "  scrypt_random_salt:               {}\n", Hexlify(&self.type2_scrypt_random_salt))              { return Ok(()); }
        if let Err(_) = write!(f, "  scrypt_log_n_factor:              {}\n", self.type2_scrypt_log_n_factor)                       { return Ok(()); }
        if let Err(_) = write!(f, "  scrypt_iteration_count:           {}\n", self.type2_scrypt_iteration_count)                    { return Ok(()); }
        if let Err(_) = write!(f, "  encrypted_identity_unlock_key:    {}\n", Hexlify(&self.type2_encrypted_identity_unlock_key))   { return Ok(()); }
        if let Err(_) = write!(f, "  verification_tag:                 {}\n", Hexlify(&self.type2_verification_tag))                { return Ok(()); }

        Ok(())
    }
}

fn read_u8_array(dest: &mut [u8], sqrlbinary: &[u8], offset: &mut usize, length: i32) {
    for i in 0..length as usize {
        dest[i] = sqrlbinary[*offset];
        *offset += 1;
    }
}

impl SqrlS4Identity {
    pub fn from_binary(sqrlbinary: &[u8]) -> SqrlS4Identity {
        let offset = &mut 0;
        //let header = sqrlbinary.read_with::<&str>(offset, Str::Len(8));

        let mut identity = SqrlS4Identity::default();
        read_u8_array(&mut identity.header, sqrlbinary, offset, 8);
        identity.type1_length =                          sqrlbinary.read_with::<u16>(offset, LE).unwrap();
        identity.type1_type =                            sqrlbinary.read_with::<u16>(offset, LE).unwrap();
        identity.type1_pt_length =                       sqrlbinary.read_with::<u16>(offset, LE).unwrap();
        read_u8_array(&mut identity.type1_aes_gcm_iv, sqrlbinary, offset, 12);
        read_u8_array(&mut identity.type1_scrypt_random_salt, sqrlbinary, offset, 16);
        identity.type1_scrypt_log_n_factor =             sqrlbinary.read_with::<u8>(offset, LE).unwrap();
        identity.type1_scrypt_iteration_count =          sqrlbinary.read_with::<u32>(offset, LE).unwrap();
        identity.type1_option_flags =                    sqrlbinary.read_with::<u16>(offset, LE).unwrap();
        identity.type1_hint_length =                     sqrlbinary.read_with::<u8>(offset, LE).unwrap();
        identity.type1_pw_verify_sec =                   sqrlbinary.read_with::<u8>(offset, LE).unwrap();
        identity.type1_idle_timeout_min =                sqrlbinary.read_with::<u16>(offset, LE).unwrap();
        read_u8_array(&mut identity.type1_encrypted_identity_master_key, sqrlbinary, offset, 32);
        read_u8_array(&mut identity.type1_encrypted_identity_lock_key, sqrlbinary, offset, 32);
        read_u8_array(&mut identity.type1_verification_tag, sqrlbinary, offset, 16);
        identity.type2_length =                          sqrlbinary.read_with::<u16>(offset, LE).unwrap();
        identity.type2_type =                            sqrlbinary.read_with::<u16>(offset, LE).unwrap();
        read_u8_array(&mut identity.type2_scrypt_random_salt, sqrlbinary, offset, 16);
        identity.type2_scrypt_log_n_factor =             sqrlbinary.read_with::<u8>(offset, LE).unwrap();
        identity.type2_scrypt_iteration_count =          sqrlbinary.read_with::<u32>(offset, LE).unwrap();
        read_u8_array(&mut identity.type2_encrypted_identity_unlock_key, sqrlbinary, offset, 32);
        read_u8_array(&mut identity.type2_verification_tag, sqrlbinary, offset, 16);

        identity
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

    //#[test]
    //#[should_panic]
    //fn test_from_binary_invalid_header() {
    //    let sqrlbinary = b"invalid_}\x00\x01\x00-\x00\"wQ\x122\x0e\xb5\x891\xfep\x97\xef\xf2e]\xf6\x0fg\x07\x8c_\xda\xd4\xe0Z\xe0\xb8\t\x96\x00\x00\x00\xf3\x01\x04\x05\x0f\x00\x023\x88\xcd\xa0\xd7WN\xf7\x8a\xd19\xf8\x1c]\x13\x87\x06\xc6\xe8\xf8\xb08\xf6\x14\xd9m\x9e\xf6|\x94\xa4\x1fF\xab}\x0e\xd3\xbf\xa3r\xa3^\xb4\xfb\xcc\xe7\x8cQ\x8d\x8dyRl\x05\xf1\x19|\x90\x03\x06\t\xe0\xb3\x85H\x8c\xe0\xa6\x0fQm\xf6\x94q6-\xee\xe0\xe9I\x00\x02\x00\xea\xde\x04q\xa1\xfaO\x8f\x1c\xf5e\xea\xb3)-^\t\xa5\x00\x00\x00\xf9o$\"\x9e\x91\xa6\xa9k\xde\xe2z^&j\xa6\x15\xb5\x04\xf4P\x01e\xcc\xfa\xa8V\xd7\xf4\x94L\xea\xea\xdd><\xcbC\xc5+\xeb\xaf\x18\x88\xf9\xa6\xd4\xce";
    //    let identity = SqrlS4Identity::from_binary(sqrlbinary);
    //    println!("{}", identity);
    //}
}
