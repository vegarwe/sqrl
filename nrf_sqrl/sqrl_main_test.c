static void print_array(const uint8_t *string, size_t len)
{
  #if NRF_LOG_ENABLED
  for(size_t i = 0; i < len; i++)
  {
    NRF_LOG_RAW_INFO("%02x", string[i]);
  }
  #endif // NRF_LOG_ENABLED
}

#define PRINT_HEX(msg, res, len)    \
do                                  \
{                                   \
    NRF_LOG_RAW_INFO(msg);          \
    NRF_LOG_RAW_INFO(" ");          \
    print_array(res, len);          \
    NRF_LOG_RAW_INFO("\n")          \
} while(0)




void test(void)
{
    NRF_LOG_RAW_INFO("\n\nEdDSA example started\n");

    // Start out with hard coded test keys
    uint8_t iuk[] = {0xa2,0x43,0xbf,0xb0,0xed,0x04,0x56,0x5d,0x04,0x61,0xa7,0x73,0x1a,0x8f,0xad,0x18,0xde,0x4b,0xdd,0xf8,0xe3,0x83,0x38,0x53,0x92,0x6c,0xcf,0xaf,0x2e,0x2e,0x04,0xa6};
    uint8_t imk[] = {0x21,0xd7,0x08,0x94,0x57,0x5e,0x6b,0x6e,0xfe,0x99,0x1f,0xb8,0x6a,0x98,0x68,0xa4,0x9f,0x3a,0x72,0x04,0x0e,0x88,0x25,0x2d,0x82,0xbe,0x5a,0x3a,0xc6,0xc3,0xaa,0x23};

    uint8_t ilk[32];
    sqrl_get_ilk_from_iuk(ilk, iuk);
    PRINT_HEX("ilk ", ilk, 32);
    NRF_LOG_RAW_INFO("ilk  00d3a56b500bca7908eb89a6f5fe0931388797d42930798d2ffe88d436c94878\n\n");

    // Get idk for site
    char sks[] = "www.grc.com";
    uint8_t ssk[32];
    uint8_t idk[32];
    sqrl_get_idk_for_site(idk, ssk, imk, sks);
    PRINT_HEX("ssk ", ssk, 32);
    NRF_LOG_RAW_INFO("ssk  a9f02ccd2ef61146a0f4c9101f4dbf285059d9687cba62b136b9a188623943d4\n");
    PRINT_HEX("idk ", idk, 32);
    NRF_LOG_RAW_INFO("idk  cd9f76fdcdbfb99a72c3f64abd318bbebbac7d2c730906369499f0b8c6bb64dd\n\n");

    // Get ursk and vuk from suk and iuk
    uint8_t suk[] = {0x0c,0x49,0xb7,0xd7,0x01,0x06,0xec,0xc3,0x3c,0x68,0xc7,0xc9,0x93,0x12,0x33,0xdf,0xc6,0x84,0x98,0xf4,0x6a,0xb7,0xda,0x4e,0xea,0x20,0xdf,0xb6,0x92,0x5e,0xc2,0x7f};
    uint8_t vuk[32];
    uint8_t ursk[32];
    sqrl_get_unlock_request_signing_key(ursk, vuk, suk, iuk);
    PRINT_HEX("suk ", suk, 32);
    NRF_LOG_RAW_INFO("suk  0c49b7d70106ecc33c68c7c9931233dfc68498f46ab7da4eea20dfb6925ec27f\n");
    PRINT_HEX("vuk ", vuk, 32);
    NRF_LOG_RAW_INFO("vuk  580a13634186a5aca95a99f94f36bad7c5aed58e3d95e00a8b63a559a5543817\n");
    PRINT_HEX("ursk", ursk, 32);
    NRF_LOG_RAW_INFO("ursk0793d0e4c49ea722e7d59b6c874f2a0198ccb53bd465c4022ab5019c14737050a\n\n");

    // Test EnHash by creating ins from sin
    char sin[] = "0";
    uint8_t ins[32];
    sqrl_get_ins_from_sin(ins, ssk, sin);
    PRINT_HEX("ins ", ins, 32);
    NRF_LOG_RAW_INFO("ins  d4389834427c0029e0919368aa0e744f85bf1157d67ef559841fe3db52ee9b93\n");

    // Test sha256
    uint8_t sha[32];
    sqrl_sha256(sha, sks, strlen(sks));
    PRINT_HEX("sha ", sha, 32);
    NRF_LOG_RAW_INFO("sha  8fa88b71d0f8efd1d0c77341f2d39b1c7c1ceac995dc13c273e6f438b04745c9\n\n");

    // Test url safe base64 encode
    char somedata[] = {0x60,0x78,0x13,0x41,0xb4,0x36,0x30,0xfb,0x6d,0x21,0x4d,0x20,0xed,0x4b,0xf8,0x77,0xaf,0xed,0x40,0xf3,0x7c,0x87,0x1c,0x06,0x13,0x89,0xbc,0xb7,0xd0,0xbe,0xe4,0x2d};
    size_t encoded_len = sqrl_base64_size_calc(somedata, sizeof(somedata));
    NRF_LOG_RAW_INFO("encoded_len %d, needed? %d\n", encoded_len, (sizeof(somedata) / 3 * 4) + 4);
    char encoded[encoded_len];
    int ret = sqrl_base64_encode(encoded, &encoded_len, somedata, sizeof(somedata));
    NRF_LOG_RAW_INFO("ret %d, encoded_len %d, sizeof(encoded) %d, sizeof(somedata) %d\n",
            ret, encoded_len, sizeof(encoded), sizeof(somedata));
    NRF_LOG_RAW_INFO("b64  %s\n", encoded);
    NRF_LOG_RAW_INFO("b640 YHgTQbQ2MPttIU0g7Uv4d6_tQPN8hxwGE4m8t9C-5C0\n\n");


    client_response_t resp;
    sqrl_cmd_t cmd;

    // Test query
    cmd.sks = sks;
    cmd.server = (char*) "c3FybDovL3d3dy5ncmMuY29tL3Nxcmw_bnV0PW9HWEVVRW1Ua1BHMHowRWthM3BISlE";
    sqrl_query(&resp, &cmd, imk);
    NRF_LOG_RAW_INFO("client: %s\n", resp.client);
    NRF_LOG_RAW_INFO("client0 dmVyPTENCmNtZD1xdWVyeQ0KaWRrPXpaOTJfYzJfdVpweXdfWkt2VEdMdnJ1c2ZTeHpDUVkybEpud3VNYTdaTjANCm9wdD1jcHN-c3VrDQo\n");
    NRF_LOG_RAW_INFO("ids:    %s\n", resp.ids);
    NRF_LOG_RAW_INFO("ids0    3Y2fcPZx6d9CuHol8b48fbHQ11tCtIiiLXqj0ZXj87J-in4kYT8RtwmTsYF5Ws5bBONah5udn5JvcKHnKMMrCQ\n\n");
    free(resp.client);
    free(resp.ids);



    // Test ident
    // TODO: Create random 32 bytes
    uint8_t rlk[32] = {0xca,0x5a,0x7b,0x6e,0xa8,0xbc,0x75,0xb3,0x94,0xd1,0xdf,0x20,0xbc,0xd9,0xcf,0x4d,0x31,0x1d,0xb0,0x67,0xd8,0x77,0xd9,0xb6,0xa7,0xda,0x74,0xd6,0x1b,0x6a,0x8d,0x69};
    cmd.sks = sks;
    cmd.server = (char*) "dmVyPTENCm51dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQp0aWY9NQ0KcXJ5PS9zcXJsP251dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzdWs9UEJGdWZRNmR2emgtYXB3dU1tXzR6MmFybmZNdjRDVUxVRTRWZVVFYWdWOA0KdXJsPWh0dHBzOi8vd3d3LmdyYy5jb20vc3FybC9kaWFnLmh0bT9fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzaW49MA0K";
    cmd.sin = "0";
    cmd.create_suk = true;
    sqrl_ident(&resp, &cmd, ilk, imk, rlk);
    NRF_LOG_RAW_INFO("client: %s\n", resp.client);
    NRF_LOG_RAW_INFO("client0 dmVyPTENCmNtZD1xdWVyeQ0KaWRrPXpaOTJfYzJfdVpweXdfWkt2VEdMdnJ1c2ZTeHpDUVkybEpud3VNYTdaTjANCmlucz0xRGlZTkVKOEFDbmdrWk5vcWc1MFQ0V19FVmZXZnZWWmhCX2oyMUx1bTVNDQpzdWs9aFRCX1d1SDU2ZlQyb1BETElldnNTMzNHYWp0UVVWOUtrWjRmUkwxOGlRaw0KdnVrPU9VWi03M2lpY1gzd0x3YlB1eDRYQjJpQ0REQ05sdXlZd3Zwdi00eHc1Rk0NCm9wdD1jcHN-c3VrDQo\n\n");


    NRF_LOG_RAW_INFO("EdDSA example executed successfully\n");
}


