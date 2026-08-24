#include "bcrypt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#define BCRYPT_WORDS 6
#define BCRYPT_CIPHERTEXT "OrpheanBeholderScryDoubt"

static const uint32_t P_INIT[18] = {
    0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344, 0xa4093822, 0x299f31d0,
    0x082efa98, 0xec4e6c89, 0x452821e6, 0x38d01377, 0xbe5466cf, 0x34e90c6c,
    0xc0ac29b7, 0xc97c50dd, 0x3f84d5b5, 0xb5470917, 0x9216d5d9, 0x8979fb1b
};

static const uint32_t S_INIT[4][256] = {
    {
        0xd1310ba6, 0x98dfb5ac, 0x2ffd72db, 0xd01adfb7, 0xb8e1afed, 0x6a267e96,
        0xba7c9045, 0xf12c7f99, 0x24a19947, 0xb3916cf7, 0x0801f2e2, 0x858efc16,
        0x636920d8, 0x71574e69, 0xa458fea3, 0xf4933d7e, 0x0d95748f, 0x728eb658,
        0x718bcd58, 0x82154aee, 0x7b54a41d, 0xc25a59b5, 0x9c30d539, 0x2af26013,
        0xc5d1b023, 0x286085f0, 0xca417918, 0xb8db38ef, 0x8e79dcb0, 0x603a180e,
        0x6c9e0e8b, 0xb01e8a3e, 0xd71577c1, 0xbd314b27, 0x78af2fda, 0x55605c60,
        0xe65525f3, 0xaa55ab94, 0x57489862, 0x63e81440, 0x55ca396a, 0x2aab10b6,
        0xb4cc5c34, 0x1141e8ce, 0xa15486af, 0x7c72e993, 0xb3ee1411, 0x636fbc2a,
        0x2ba9c55d, 0x741831f6, 0xce5c3e16, 0x9b87931e, 0xafd6ba33, 0x6c24cf5c,
        0x7a325381, 0x28958677, 0x3b8f4898, 0x6b4bb9af, 0xc4bfe81b, 0x66282193,
        0x61d809cc, 0xfb21a991, 0x487cac60, 0x5dec8032, 0xef845d5d, 0xe98575b1,
        0xdc262302, 0xeb651b88, 0x23893e81, 0xd396acc5, 0x0f6d6ff3, 0x83f44239,
        0x2e0b4482, 0xa4842004, 0x69c8f04a, 0x9e1f9b5e, 0x21c66842, 0xf6e96c9a,
        0x670c9c61, 0xabd388f0, 0x6a51a0d2, 0xd8542f68, 0x960fa728, 0xab5133a3,
        0x6eef0b6c, 0x137a3be4, 0xba3bf050, 0x7efb2bbe, 0x9b1147b7, 0x90957605,
        0x9e72b49c, 0xa93e2f84, 0x7365eef2, 0x96102fd1, 0xccde40d4, 0x29ac09e8,
        0xb62fb183, 0x603507e0, 0x308c841f, 0x1e79ff3d, 0x37b08302, 0xbbf5af04,
        0x4fb9a6e0, 0x5e794b8f, 0x40e721fe, 0xd4cd6fa9, 0xdab8644f, 0xd0e8d750,
        0x0c83e1d2, 0x999f725f, 0x6ecf14f0, 0x98444022, 0xee4080e6, 0x85b331f0,
        0x8fe1076a, 0x301095ee, 0x15e094c4, 0x05f14e22, 0x4e439621, 0xa7566d3a,
        0x7e9da9e0, 0xeb068050, 0x48cfce99, 0x92ae4573, 0x1b11e3b2, 0x31929ab7,
        0x7ab63e10, 0x66a0c006, 0x13bece6a, 0x948140f7, 0xfabb7131, 0x316e0450,
        0xfa5e47f2, 0x629730e1, 0x7748f484, 0x6e969b45, 0x54407603, 0x08ec39e2,
        0x4975a467, 0xa3d3d45a, 0x80674dcc, 0x809d0121, 0xb550da37, 0x72430e63,
        0xfe0f1a04, 0x9861fe58, 0x8050965b, 0x64d9541d, 0x9e47f9d4, 0x539c30c8,
        0x4190672e, 0x2c475474, 0x82ab6441, 0x46d6b2fe, 0x2b0f6997, 0x8ab3382b,
        0xcd23ff7e, 0x3ac45766, 0x49ab02fb, 0x4940b5b9, 0x42885400, 0xbbdd9322,
        0xf741ec7f, 0x40445a5e, 0x20ed99a4, 0x77397663, 0x32ec130f, 0x6e10d7b2,
        0xb0d8174f, 0x6b088836, 0x59be09e8, 0xea8f5bbf, 0xac4f8361, 0xc0a2890d,
        0xa9961d69, 0x9771164d, 0x498d76b4, 0x08264c0e, 0x3648348b, 0xd0f84a80,
        0x159b55b3, 0x494c16a6, 0x4796ad25, 0xa8413b61, 0x8fb895cf, 0x974c90e2,
        0x770ac416, 0x7eedc72d, 0xfa505071, 0x90934941, 0x0156a23d, 0x59970e94,
        0x6a6d65be, 0x2ff03409, 0xceac96c2, 0x764dd4e0, 0xa53f45f9, 0x53f1433b,
        0x792018d2, 0xb25ddf44, 0xd73f15b0, 0x67026081, 0x4f4f45ab, 0x869bc283,
        0x20f0fffb, 0x55d3ea45, 0x92120e2f, 0xc7f66ff3, 0x6e3756ef, 0x46427d19,
        0x6d8d3499, 0x321871e3, 0x8794a092, 0xf7a9f533, 0x88214f10, 0x838f71a5,
        0xac300a7d, 0x48a7130b, 0x495f5c47, 0x79fbe5ab, 0xbbe9bfe0, 0x41415aac,
        0x60e0a084, 0x5dadd835, 0xafe9e7d2, 0xf1290dc7, 0x7257ade7, 0x9330d1b2,
        0x4f6ad7f0, 0x0c29407f, 0x67949b08, 0xbebf6360, 0xdf634f37, 0x750c4f34,
        0x9ee4d9b2, 0xf02fbe34, 0x823542eb, 0x5b564564, 0xac75ec61, 0x4d33a9c8,
        0x08d55239, 0xd3383012, 0x04507a33, 0x409d9b5f, 0xdd2101c4, 0xb663f940,
        0x6e1298e9, 0x19b24cd2, 0x224e73d2, 0x3f86d0e0, 0x613f6663, 0x5f64939d
    },
    {
        0x23f11d35, 0x31796111, 0x840ec5b8, 0xc1250a89, 0x6635c52f, 0x32473344,
        0xb160e75e, 0x46495c1f, 0x4d7e2d48, 0x08befecf, 0x1b0f9ad7, 0x2c0690ce,
        0x416bbf70, 0xb2721869, 0x4f391f04, 0xec89b533, 0x359b04e2, 0x75746e1d,
        0x9926a45f, 0x1d8189fb, 0xebb33112, 0x85e5922e, 0x2f1b88db, 0x401a63a8,
        0x4d0cddb0, 0x8a8a79a9, 0xbfb73222, 0x0364ec4f, 0x807a242b, 0x38d6049f,
        0x466d2315, 0x780e8147, 0x4604d612, 0x23496783, 0x7da72d4e, 0xad64905b,
        0xa5e25278, 0x86054f70, 0x94643d04, 0x2e5c0a39, 0x4b7e0450, 0x2697f245,
        0x640fc824, 0x40f12145, 0xa01da0ee, 0x41848132, 0xb0f0376a, 0x512183f3,
        0x5a66ee13, 0x0e6940e7, 0xeb9e8e6e, 0x479c4f5a, 0x25869e47, 0x321a9039,
        0xa012d807, 0xb7ac7f36, 0x70594194, 0x32614380, 0x47564156, 0x6471fe6f,
        0xc723e35f, 0x07fb37a5, 0x36a79aac, 0x06eb30d9, 0xa31662e7, 0x6ced9c14,
        0xad6ee801, 0x4e0fa9be, 0x52299f24, 0x0cf7e250, 0xbe613237, 0xbf51dc20,
        0x9e1140f6, 0xd1a59efb, 0x5fb77883, 0x4052c7f2, 0xd6e24611, 0x0fe67d32,
        0xcdbf6316, 0xb45715be, 0x141065c0, 0x7773bc37, 0x6eecb12c, 0x1dae96d3,
        0xf7e25014, 0x03134723, 0xa37964f4, 0x5ca3db7d, 0x863647d6, 0x5211832d,
        0x33b98d40, 0xef608095, 0x64421f3a, 0x0cbdd994, 0x328d84db, 0x3964547b,
        0xee444215, 0x42e7aecb, 0x41e2511d, 0x956a449e, 0x257e103a, 0x93751b23,
        0x1fbfb6ec, 0x6854ec0a, 0x51fb84a3, 0xd479d232, 0xa1647c3d, 0xbda50d11,
        0x995b4522, 0xd12fea42, 0xecda4d44, 0x05317aa8, 0x683fe615, 0x68126e60,
        0x5a872323, 0x78642143, 0xa61019e6, 0xec2315b7, 0xeb559382, 0xd4683061,
        0x784b02d0, 0x159e51ba, 0x9f7b402b, 0xa0c2ee1b, 0x43c7d460, 0x9d2949e4,
        0x4f69c87f, 0x600f7bb0, 0x2030faef, 0x2c416124, 0x456da5a8, 0x2bd60e6f,
        0xb030f304, 0x16665738, 0x2b0430e4, 0x00eb4501, 0x4c042d23, 0xe73d2f00,
        0x804728be, 0x5affb2b1, 0x4cf54330, 0x02c6c60a, 0x42bf19b6, 0x9e314080,
        0xcbf4d24d, 0x389f4376, 0x6c838082, 0x5d39424b, 0x4feb1420, 0x633e3f00,
        0x231a479f, 0x655b66d0, 0x40139428, 0x9982063c, 0x97534f53, 0x6a008d02,
        0x3642a0d9, 0x256079fd, 0x0470d061, 0x3564243b, 0xb4577f5e, 0x02fbb82f,
        0x3de82935, 0x8a258986, 0xeceb1720, 0x2c5b7ffc, 0xb6166b40, 0x5a4ad7f8,
        0x2ff60c69, 0x0f4e6203, 0xa7c4841a, 0x31bfd00e, 0x9457ee6f, 0x4642699f,
        0x1230d32f, 0x02e1183b, 0x6ec221e9, 0x2224641b, 0x3e4766cf, 0xdfe625e6,
        0x2b732881, 0xa6917052, 0x6ed0a281, 0x5ce4107f, 0xa5fe3fd1, 0x037042b9,
        0x86497f3c, 0x68310a5c, 0x8bb89d6e, 0x6612f5e1, 0x9ddac247, 0xe29b846e,
        0x4f71e35c, 0x47ebd935, 0x1cd62960, 0x6cfb5e41, 0x2d64216e, 0x826fad3f,
        0xec34538e, 0xbe708051, 0x73887f7a, 0x1e63d953, 0x6f1516f2, 0x605e47b0,
        0x49f166b6, 0x164b526d, 0x303105a8, 0x51710f6b, 0x43165a00, 0x33332d03,
        0x4cfb03f6, 0x19defb28, 0x4d6f5219, 0xae20e220, 0x12022023, 0x4c11414f,
        0x676ecb2e, 0x2c62c1d8, 0x993fefd6, 0xf2de4381, 0xc0f57c99, 0x2d1e6216,
        0x286e0741, 0xa308d312, 0x19a8c2fe, 0x2875d290, 0x50f0ec41, 0xe909fc06,
        0x82d9f289, 0xda26e422, 0x4d0e5b6a, 0x0761f0e8, 0x96e041f6, 0x47702040,
        0x1a041825, 0x80b4e284, 0x416e0191, 0x53805202, 0xeaa7fed9, 0x63dc1a05,
        0x94610026, 0x41326420, 0x3d434a72, 0xb66330b6, 0x20b6e4e0, 0x8839743f,
        0x53421d00, 0x526b1fcd, 0x33ccb07c, 0x3965217a, 0x0e654a99, 0x5b5e2021
    },
    {
        0x3bf72a87, 0x6730c430, 0xc3fe94f1, 0x896432b0, 0x577038d8, 0xe50c96f0,
        0x10b1045e, 0x24336c19, 0x1af72a04, 0x879560f1, 0x8771e652, 0xbce1421f,
        0xbd4f4340, 0x5f362624, 0x6efc6c46, 0xa1f2b5bb, 0x6767c096, 0x18047fcd,
        0x9912a20f, 0xc5161a0a, 0xeb80885f, 0x156eb6c7, 0x49ec36f5, 0x07fcdf7f,
        0x86733ecb, 0xbcc0c58c, 0x9d3f2d0f, 0x450b7e8d, 0xd05a2b8c, 0x423f0013,
        0xc776743f, 0x838750e2, 0xc9e2dacd, 0x08b57523, 0x61e5a0f0, 0xa0647d08,
        0xc25701c3, 0x06059a3f, 0x5b35d024, 0x4c44a049, 0xf7be3775, 0xadd7432d,
        0x7a2c4782, 0x49ee5b99, 0xe45e1d40, 0x75701546, 0xf97e1c73, 0x421f7512,
        0x0997ee1b, 0x0e89d550, 0x0f4005b2, 0x323c1037, 0xd89470f2, 0x93302e76,
        0x45a0d4e1, 0x4667a61d, 0x738564f1, 0xb4e30fe6, 0x188a4208, 0x64c9bc4f,
        0xd2c4e741, 0x8a679c2b, 0xe72d438d, 0xb878396e, 0x8cfc16bb, 0x11e69341,
        0x02c01493, 0x65613413, 0xbaab483d, 0x608b4e62, 0x41e65b80, 0xa37b3724,
        0x41f6e53c, 0x35962613, 0xc6e24682, 0x29c95d52, 0x6ca51cb4, 0x54556486,
        0x1ab29f47, 0x4ceb4fcd, 0x6ee2d103, 0xd423d088, 0x40c74251, 0x1d63c3fe,
        0x4d2f1402, 0xd9e2da4c, 0xd476a812, 0xb1a50b05, 0x65aa0b24, 0xae4200a4,
        0xf74e2435, 0x554e7069, 0x0858973a, 0x47e4524a, 0x911e94fc, 0x96480000,
        0x1973e24d, 0x60943f8b, 0xf4d60c2f, 0x738b1324, 0xdcd64706, 0x4543d449,
        0x65142b1b, 0x97a04f41, 0xecb9e195, 0x4a79a770, 0xa0f0ed88, 0x422002b0,
        0x446061fe, 0x4139f2d0, 0xbcb133cc, 0x97b74140, 0x4226d052, 0x18ec9a5b,
        0xdb6059f5, 0x04b7262f, 0x721f46fc, 0x40076018, 0x31bf7158, 0x8054410b,
        0x71437b64, 0xfcfd964e, 0x47e01201, 0x76b4d76f, 0x1c03e080, 0x7e2059b7,
        0xbc77788e, 0xe35e7ab6, 0x608a1e20, 0xa0420080, 0x428a4fe0, 0xd0b0ce91,
        0x046b0102, 0xf44a4215, 0xb4e11266, 0x3e11ecce, 0xd6b300e2, 0x9824e4f8,
        0xc0c1ecf1, 0x44a24c0f, 0x4cf9f9d2, 0x8d515096, 0x64241042, 0x15e8dd50,
        0x52897f1b, 0x4215e4f2, 0xa6079c5f, 0x3dbee830, 0xfecd931b, 0x6cac629d,
        0xf4311409, 0xf3564105, 0x1ef61250, 0x397b2fe1, 0x2b513105, 0xfde8063f,
        0x53118226, 0x5cbd4e06, 0x4067ef14, 0x4dd59872, 0x76e3c0f9, 0xbbe44976,
        0x543e4305, 0x56f6146f, 0x64a3140f, 0x6bfe13be, 0x342f6d4b, 0x9830f022,
        0x91124d3c, 0xa2015b14, 0x61206cf8, 0x5330e21c, 0x85324d3b, 0xd211319c,
        0x73a80373, 0x40f4135f, 0x9b5f7901, 0x514fdb20, 0x2fe5447b, 0x1e31e243,
        0x5fbac2f4, 0x6ae4d699, 0x3d70b93c, 0x4410e666, 0x49308300, 0x711b836f,
        0x476d00f8, 0x114e02fe, 0x751e3328, 0x80e41f1b, 0x54b11e9f, 0x24141690,
        0x83e1551e, 0x05f77770, 0x18e979d2, 0x80ae161a, 0x05eb256b, 0x53313013,
        0x01f14419, 0x4d341400, 0x4bb0f721, 0x534e03b3, 0x56f830c8, 0x4f1155c0,
        0xf17ee250, 0x108c0b4f, 0xba085547, 0x46b4effb, 0x01ee1d00, 0x7ceb821b,
        0x4f7d7614, 0x47e9b205, 0x751b15a3, 0x4f222d01, 0x6b1e50be, 0x6a45b329,
        0x4a416e08, 0xa5126694, 0x5e164e63, 0xbd206070, 0x321e27a0, 0x43912613,
        0x3e22e49e, 0x65dc6016, 0x3ef87f7e, 0xd301a680, 0x45da2614, 0x016e6f84,
        0x4377be1d, 0xd16e0f34, 0x87126200, 0x647d2166, 0x31e020c8, 0xecbe70c4,
        0x40a70820, 0x05bca490, 0x4ef38fe2, 0xd0e41c49, 0x4ddfeff6, 0x353f4f4f,
        0x98f725be, 0xed5188a6, 0xefc00e6d, 0x3da4d1b0, 0x54043000, 0xa2f400a6,
        0xf71e6601, 0x42406245, 0xa062415a, 0x81a0d4ff, 0x45b622f3, 0x54a53082
    },
    {
        0x6730c430, 0xc3fe94f1, 0x896432b0, 0xfe38d843, 0x59970e94, 0x64c9bc4f,
        0xd2c4e741, 0x8a679c2b, 0xe72d438d, 0xb878396e, 0x8cfc16bb, 0x11e69341,
        0x02c01493, 0x65613413, 0xbaab483d, 0x608b4e62, 0x41e65b80, 0xa37b3724,
        0x41f6e53c, 0x35962613, 0xc6e24682, 0x29c95d52, 0x6ca51cb4, 0x54556486,
        0x1ab29f47, 0x4ceb4fcd, 0x6ee2d103, 0xd423d088, 0x40c74251, 0x1d63c3fe,
        0x4d2f1402, 0xd9e2da4c, 0xd476a812, 0xb1a50b05, 0x65aa0b24, 0xae4200a4,
        0xf74e2435, 0x554e7069, 0x0858973a, 0x47e4524a, 0x911e94fc, 0x96480000,
        0x1973e24d, 0x60943f8b, 0xf4d60c2f, 0x738b1324, 0xdcd64706, 0x4543d449,
        0x65142b1b, 0x97a04f41, 0xecb9e195, 0x4a79a770, 0xa0f0ed88, 0x422002b0,
        0x446061fe, 0x4139f2d0, 0xbcb133cc, 0x97b74140, 0x4226d052, 0x18ec9a5b,
        0xdb6059f5, 0x04b7262f, 0x721f46fc, 0x40076018, 0x31bf7158, 0x8054410b,
        0x71437b64, 0xfcfd964e, 0x47e01201, 0x76b4d76f, 0x1c03e080, 0x7e2059b7,
        0xbc77788e, 0xe35e7ab6, 0x608a1e20, 0xa0420080, 0x428a4fe0, 0xd0b0ce91,
        0x046b0102, 0xf44a4215, 0xb4e11266, 0x3e11ecce, 0xd6b300e2, 0x9824e4f8,
        0xc0c1ecf1, 0x44a24c0f, 0x4cf9f9d2, 0x8d515096, 0x64241042, 0x15e8dd50,
        0x52897f1b, 0x4215e4f2, 0xa6079c5f, 0x3dbee830, 0xfecd931b, 0x6cac629d,
        0xf4311409, 0xf3564105, 0x1ef61250, 0x397b2fe1, 0x2b513105, 0xfde8063f,
        0x53118226, 0x5cbd4e06, 0x4067ef14, 0x4dd59872, 0x76e3c0f9, 0xbbe44976,
        0x543e4305, 0x56f6146f, 0x64a3140f, 0x6bfe13be, 0x342f6d4b, 0x9830f022,
        0x91124d3c, 0xa2015b14, 0x61206cf8, 0x5330e21c, 0x85324d3b, 0xd211319c,
        0x73a80373, 0x40f4135f, 0x9b5f7901, 0x514fdb20, 0x2fe5447b, 0x1e31e243,
        0x5fbac2f4, 0x6ae4d699, 0x3d70b93c, 0x4410e666, 0x49308300, 0x711b836f,
        0x476d00f8, 0x114e02fe, 0x751e3328, 0x80e41f1b, 0x54b11e9f, 0x24141690,
        0x83e1551e, 0x05f77770, 0x18e979d2, 0x80ae161a, 0x05eb256b, 0x53313013,
        0x01f14419, 0x4d341400, 0x4bb0f721, 0x534e03b3, 0x56f830c8, 0x4f1155c0,
        0xf17ee250, 0x108c0b4f, 0xba085547, 0x46b4effb, 0x01ee1d00, 0x7ceb821b,
        0x4f7d7614, 0x47e9b205, 0x751b15a3, 0x4f222d01, 0x6b1e50be, 0x6a45b329,
        0x4a416e08, 0xa5126694, 0x5e164e63, 0xbd206070, 0x321e27a0, 0x43912613,
        0x3e22e49e, 0x65dc6016, 0x3ef87f7e, 0xd301a680, 0x45da2614, 0x016e6f84,
        0x4377be1d, 0xd16e0f34, 0x87126200, 0x647d2166, 0x31e020c8, 0xecbe70c4,
        0x40a70820, 0x05bca490, 0x4ef38fe2, 0xd0e41c49, 0x4ddfeff6, 0x353f4f4f,
        0x98f725be, 0xed5188a6, 0xefc00e6d, 0x3da4d1b0, 0x54043000, 0xa2f400a6,
        0xf71e6601, 0x42406245, 0xa062415a, 0x81a0d4ff, 0x45b622f3, 0x54a53082,
        0xd1310ba6, 0x98dfb5ac, 0x2ffd72db, 0xd01adfb7, 0xb8e1afed, 0x6a267e96,
        0xba7c9045, 0xf12c7f99, 0x24a19947, 0xb3916cf7, 0x0801f2e2, 0x858efc16,
        0x636920d8, 0x71574e69, 0xa458fea3, 0xf4933d7e, 0x0d95748f, 0x728eb658,
        0x718bcd58, 0x82154aee, 0x7b54a41d, 0xc25a59b5, 0x9c30d539, 0x2af26013,
        0xc5d1b023, 0x286085f0, 0xca417918, 0xb8db38ef, 0x8e79dcb0, 0x603a180e,
        0x6c9e0e8b, 0xb01e8a3e, 0xd71577c1, 0xbd314b27, 0x78af2fda, 0x55605c60,
        0xe65525f3, 0xaa55ab94, 0x57489862, 0x63e81440, 0x55ca396a, 0x2aab10b6,
        0xb4cc5c34, 0x1141e8ce, 0xa15486af, 0x7c72e993, 0xb3ee1411, 0x636fbc2a,
        0x2ba9c55d, 0x741831f6, 0xce5c3e16, 0x9b87931e, 0xafd6ba33, 0x6c24cf5c
    }
};

static const char BCRYPT_BASE64[65] =
    "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static const uint8_t BCRYPT_BASE64_REV[128] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,   0,   1,
     54,  55,  56,  57,  58,  59,  60,  61,  62,  63, 255, 255, 255, 255, 255, 255,
    255,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,
     17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27, 255, 255, 255, 255, 255,
    255,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,
     43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53, 255, 255, 255, 255, 255
};

typedef struct {
    uint32_t P[18];
    uint32_t S[4][256];
} BlowfishContext;

static inline uint32_t blowfish_F(BlowfishContext *ctx, uint32_t x) {
    uint16_t a = (x >> 24) & 0xFF;
    uint16_t b = (x >> 16) & 0xFF;
    uint16_t c = (x >> 8) & 0xFF;
    uint16_t d = x & 0xFF;
    return ((ctx->S[0][a] + ctx->S[1][b]) ^ ctx->S[2][c]) + ctx->S[3][d];
}

static void blowfish_encipher(BlowfishContext *ctx, uint32_t *xl, uint32_t *xr) {
    uint32_t l = *xl;
    uint32_t r = *xr;

    for (int i = 0; i < 16; i += 2) {
        l ^= ctx->P[i];
        r ^= blowfish_F(ctx, l);
        r ^= ctx->P[i + 1];
        l ^= blowfish_F(ctx, r);
    }
    l ^= ctx->P[16];
    r ^= ctx->P[17];

    *xl = r;
    *xr = l;
}

static void blowfish_init_state(BlowfishContext *ctx) {
    memcpy(ctx->P, P_INIT, sizeof(ctx->P));
    memcpy(ctx->S, S_INIT, sizeof(ctx->S));
}

static void eksblowfish_expand(BlowfishContext *ctx, const uint8_t *salt, size_t salt_len, const uint8_t *key, size_t key_len) {
    size_t k = 0;
    for (int i = 0; i < 18; i++) {
        uint32_t data = 0;
        for (int j = 0; j < 4; j++) {
            data = (data << 8) | (key_len > 0 ? key[k % key_len] : 0);
            k++;
        }
        ctx->P[i] ^= data;
    }

    uint32_t l = 0, r = 0;
    size_t s = 0;
    for (int i = 0; i < 18; i += 2) {
        if (salt_len > 0) {
            uint32_t sd_l = 0, sd_r = 0;
            for (int j = 0; j < 4; j++) {
                sd_l = (sd_l << 8) | salt[s % salt_len];
                s++;
                sd_r = (sd_r << 8) | salt[s % salt_len];
                s++;
            }
            l ^= sd_l;
            r ^= sd_r;
        }
        blowfish_encipher(ctx, &l, &r);
        ctx->P[i] = l;
        ctx->P[i + 1] = r;
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 256; j += 2) {
            if (salt_len > 0) {
                uint32_t sd_l = 0, sd_r = 0;
                for (int m = 0; m < 4; m++) {
                    sd_l = (sd_l << 8) | salt[s % salt_len];
                    s++;
                    sd_r = (sd_r << 8) | salt[s % salt_len];
                    s++;
                }
                l ^= sd_l;
                r ^= sd_r;
            }
            blowfish_encipher(ctx, &l, &r);
            ctx->S[i][j] = l;
            ctx->S[i][j + 1] = r;
        }
    }
}

static void bcrypt_base64_encode(char *dst, const uint8_t *src, size_t src_len) {
    size_t i = 0;
    while (i < src_len) {
        uint32_t c1 = src[i++];
        uint32_t c2 = (i < src_len) ? src[i++] : 0;
        *dst++ = BCRYPT_BASE64[(c1 >> 2) & 0x3F];
        uint32_t c3 = (i < src_len) ? src[i++] : 0;
        *dst++ = BCRYPT_BASE64[((c1 & 0x03) << 4) | ((c2 >> 4) & 0x0F)];
        if (i - 1 > src_len) break;
        *dst++ = BCRYPT_BASE64[((c2 & 0x0F) << 2) | ((c3 >> 6) & 0x03)];
        if (i > src_len) break;
        *dst++ = BCRYPT_BASE64[c3 & 0x3F];
    }
    *dst = '\0';
}

static int bcrypt_base64_decode(uint8_t *dst, const char *src, size_t dst_len) {
    size_t src_len = strlen(src);
    size_t i = 0, j = 0;
    while (i < src_len && j < dst_len) {
        uint8_t c1 = (uint8_t)src[i++];
        uint8_t c2 = (i < src_len) ? (uint8_t)src[i++] : 0;
        uint8_t c3 = (i < src_len) ? (uint8_t)src[i++] : 0;
        uint8_t c4 = (i < src_len) ? (uint8_t)src[i++] : 0;

        if (c1 >= 128 || c2 >= 128 || c3 >= 128 || c4 >= 128) return 0;
        uint8_t v1 = BCRYPT_BASE64_REV[c1];
        uint8_t v2 = BCRYPT_BASE64_REV[c2];
        uint8_t v3 = BCRYPT_BASE64_REV[c3];
        uint8_t v4 = BCRYPT_BASE64_REV[c4];
        if (v1 == 255 || v2 == 255) return 0;

        if (j < dst_len) dst[j++] = (v1 << 2) | (v2 >> 4);
        if (v3 != 255 && j < dst_len) dst[j++] = (v2 << 4) | (v3 >> 2);
        if (v4 != 255 && j < dst_len) dst[j++] = (v3 << 6) | v4;
    }
    return 1;
}

int bcrypt_gensalt(int cost, char *salt) {
    if (cost < 4 || cost > 31) cost = BCRYPT_DEFAULT_COST;
    uint8_t raw_salt[16];

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        if (read(fd, raw_salt, sizeof(raw_salt)) != sizeof(raw_salt)) {
            for (int i = 0; i < 16; i++) raw_salt[i] = rand() % 256;
        }
        close(fd);
    } else {
        for (int i = 0; i < 16; i++) raw_salt[i] = rand() % 256;
    }

    char encoded_salt[32];
    bcrypt_base64_encode(encoded_salt, raw_salt, sizeof(raw_salt));
    encoded_salt[22] = '\0'; // 16 bytes base64 encoded in bcrypt = 22 characters

    snprintf(salt, 30, "$2a$%02d$%s", cost, encoded_salt);
    return 1;
}

static void bcrypt_hash_internal(const char *password, const uint8_t *raw_salt, int cost, char *out_hash) {
    BlowfishContext ctx;
    blowfish_init_state(&ctx);

    size_t pass_len = strlen(password);
    if (pass_len > 72) pass_len = 72; // Bcrypt password length limit

    // EksBlowfishSetup
    eksblowfish_expand(&ctx, raw_salt, 16, (const uint8_t*)password, pass_len + 1);

    uint32_t rounds = 1U << cost;
    for (uint32_t i = 0; i < rounds; i++) {
        eksblowfish_expand(&ctx, NULL, 0, (const uint8_t*)password, pass_len + 1);
        eksblowfish_expand(&ctx, NULL, 0, raw_salt, 16);
    }

    // Encrypt "OrpheanBeholderScryDoubt" 64 times
    uint32_t ctext[6];
    memcpy(ctext, BCRYPT_CIPHERTEXT, 24);
    for (int i = 0; i < 6; i++) {
        uint32_t val = 0;
        for (int j = 0; j < 4; j++) {
            val = (val << 8) | ((uint8_t*)BCRYPT_CIPHERTEXT)[i * 4 + j];
        }
        ctext[i] = val;
    }

    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 6; j += 2) {
            blowfish_encipher(&ctx, &ctext[j], &ctext[j + 1]);
        }
    }

    uint8_t raw_hash[24];
    for (int i = 0; i < 6; i++) {
        raw_hash[i * 4 + 0] = (ctext[i] >> 24) & 0xFF;
        raw_hash[i * 4 + 1] = (ctext[i] >> 16) & 0xFF;
        raw_hash[i * 4 + 2] = (ctext[i] >> 8) & 0xFF;
        raw_hash[i * 4 + 3] = ctext[i] & 0xFF;
    }

    char encoded_hash[40];
    bcrypt_base64_encode(encoded_hash, raw_hash, 23); // 23 bytes -> 31 characters
    encoded_hash[31] = '\0';

    char encoded_salt[32];
    bcrypt_base64_encode(encoded_salt, raw_salt, 16);
    encoded_salt[22] = '\0';

    snprintf(out_hash, BCRYPT_HASHSIZE, "$2a$%02d$%s%s", cost, encoded_salt, encoded_hash);
}

int bcrypt_hash(const char *password, char *out_hash) {
    char salt_str[32];
    bcrypt_gensalt(BCRYPT_DEFAULT_COST, salt_str);

    int cost = 10;
    sscanf(salt_str, "$2a$%d$", &cost);

    uint8_t raw_salt[16];
    bcrypt_base64_decode(raw_salt, salt_str + 7, 16);

    bcrypt_hash_internal(password, raw_salt, cost, out_hash);
    return 1;
}

int bcrypt_checkpw(const char *password, const char *hash) {
    if (!password || !hash || strlen(hash) < 28) return -1;

    int cost = 10;
    if (strncmp(hash, "$2a$", 4) != 0 && strncmp(hash, "$2b$", 4) != 0) {
        // Fallback check if legacy unhashed
        return strcmp(password, hash);
    }

    if (sscanf(hash + 4, "%d$", &cost) != 1) return -1;
    if (cost < 4 || cost > 31) return -1;

    char salt_b64[23];
    strncpy(salt_b64, hash + 7, 22);
    salt_b64[22] = '\0';

    uint8_t raw_salt[16];
    if (!bcrypt_base64_decode(raw_salt, salt_b64, 16)) return -1;

    char computed_hash[BCRYPT_HASHSIZE];
    bcrypt_hash_internal(password, raw_salt, cost, computed_hash);

    // Constant-time string comparison
    size_t len = strlen(hash);
    if (strlen(computed_hash) != len) return -1;

    int diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= (computed_hash[i] ^ hash[i]);
    }

    return (diff == 0) ? 0 : -1;
}
