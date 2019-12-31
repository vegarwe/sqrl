mod sqrl_s4;

use sqrl_s4::SqrlS4Identity;

fn main() {
    //let identity = SqrlS4Identity{type1_length: 34, ..Default::default()};
    //let mut identity = SqrlS4Identity::default();
    let sqrlbinary = b"sqrldata}\x00\x01\x00-\x00\"wQ\x122\x0e\xb5\x891\xfep\x97\xef\xf2e]\xf6\x0fg\x07\x8c_\xda\xd4\xe0Z\xe0\xb8\t\x96\x00\x00\x00\xf3\x01\x04\x05\x0f\x00\x023\x88\xcd\xa0\xd7WN\xf7\x8a\xd19\xf8\x1c]\x13\x87\x06\xc6\xe8\xf8\xb08\xf6\x14\xd9m\x9e\xf6|\x94\xa4\x1fF\xab}\x0e\xd3\xbf\xa3r\xa3^\xb4\xfb\xcc\xe7\x8cQ\x8d\x8dyRl\x05\xf1\x19|\x90\x03\x06\t\xe0\xb3\x85H\x8c\xe0\xa6\x0fQm\xf6\x94q6-\xee\xe0\xe9I\x00\x02\x00\xea\xde\x04q\xa1\xfaO\x8f\x1c\xf5e\xea\xb3)-^\t\xa5\x00\x00\x00\xf9o$\"\x9e\x91\xa6\xa9k\xde\xe2z^&j\xa6\x15\xb5\x04\xf4P\x01e\xcc\xfa\xa8V\xd7\xf4\x94L\xea\xea\xdd><\xcbC\xc5+\xeb\xaf\x18\x88\xf9\xa6\xd4\xce";
    let mut identity = SqrlS4Identity::from_binary(sqrlbinary);
    println!("identity debug\n{:?}", identity);
    println!("identity print\n{}", identity);

    identity.type1_length = 125;
    println!("identity.type1_length {}", identity.type1_length);

}

//Type1: user access password protected data
//  magic:                            sqrldata,
//  length:                           125,
//  type:                             1,
//  pt_length:                        45,
//  aes_gcm_iv:                       b'22775112320eb58931fe7097',
//  scrypt_random_salt:               b'eff2655df60f67078c5fdad4e05ae0b8',
//  scrypt_log_n_factor:              9,
//  scrypt_iteration_count:           150,
//  option_flags:                     499,
//  hint_length:                      4,
//  pw_verify_sec:                    5,
//  idle_timeout_min:                 15,
//  encrypted_identity_master_key:    b'023388cda0d7574ef78ad139f81c5d138706c6e8f8b038f614d96d9ef67c94a4',
//  encrypted_identity_lock_key:      b'1f46ab7d0ed3bfa372a35eb4fbcce78c518d8d79526c05f1197c90030609e0b3',
//  verification_tag:                 b'85488ce0a60f516df69471362deee0e9'
//Type2: rescue code data
//  length:                           73,
//  type:                             2,
//  scrypt_random_salt:               b'eade0471a1fa4f8f1cf565eab3292d5e',
//  scrypt_log_n_factor:              9,
//  scrypt_iteration_count:           165,
//  encrypted_identity_unlock_key:    b'f96f24229e91a6a96bdee27a5e266aa615b504f4500165ccfaa856d7f4944cea',
//  verification_tag:                 b'eadd3e3ccb43c52bebaf1888f9a6d4ce'
