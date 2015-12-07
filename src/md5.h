/* MD5.H - header file for MD5C.C
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
 */

/* MD5 context. */
#if !defined(_MD5_H_)
#define _MD5_H_

#define u32 unsigned long 
#define u16 unsigned short
#define u8 unsigned char

typedef struct {
  u32 state[4];                                   /* state (ABCD) */
  u32 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  u8 buffer[64];                         /* input buffer */
} MD5_CTX;


#ifdef __cplusplus
extern "C" {
#endif

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, u8 *, u32);
void MD5String(MD5_CTX *, const char *str);
void MD5Final(u8 [16], MD5_CTX *);

#ifdef __cplusplus
}

class MD5Digest
{
	MD5_CTX context;
	u8 digest[16];
public:
	void initialize() {MD5Init(&context); memset(digest, 0, 16);}
	MD5Digest() {initialize();}
	void update(const u8 *bin, u32 size)
	{MD5Update(&context, const_cast<u8 *>((const u8 *)bin), size);}
	void string(const char *str) {MD5String(&context, str);}
	void final() {MD5Final(digest, &context);}
	void final(u8 *buffer) {final(); memcpy(buffer, digest, 16);}
	const u8 *getDigest() const {return digest;}
	unsigned long operator[](int n) const {return digest[n];}
};
#endif

#endif

