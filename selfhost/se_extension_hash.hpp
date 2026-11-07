#pragma once
// Standalone SHA-256 for SE self-hosted code generation.
// FIPS 180-4; no OpenSSL dependency or operating-system subprocess.
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

class SESha256 {
  static constexpr std::array<std::uint32_t,64> round_constants_{
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
    0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
    0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
    0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
    0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
    0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
    0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
    0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
    0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
  };
  std::array<std::uint32_t,8> h_{
    0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
    0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
  };
  std::array<std::uint8_t,64> buffer_{};
  std::size_t used_=0;
  std::uint64_t bytes_=0;
  bool finished_=false;

  static constexpr std::uint32_t rotate(std::uint32_t value,unsigned bits) {
    return (value >> bits) | (value << (32-bits));
  }
  void compress(const std::uint8_t* block) {
    std::array<std::uint32_t,64> words{};
    for(std::size_t i=0;i<16;++i)
      words[i]=(std::uint32_t(block[i*4])<<24)|
               (std::uint32_t(block[i*4+1])<<16)|
               (std::uint32_t(block[i*4+2])<<8)|
               std::uint32_t(block[i*4+3]);
    for(std::size_t i=16;i<64;++i) {
      const auto s0=rotate(words[i-15],7)^rotate(words[i-15],18)^(words[i-15]>>3);
      const auto s1=rotate(words[i-2],17)^rotate(words[i-2],19)^(words[i-2]>>10);
      words[i]=words[i-16]+s0+words[i-7]+s1;
    }
    auto a=h_[0],b=h_[1],c=h_[2],d=h_[3];
    auto e=h_[4],f=h_[5],g=h_[6],h=h_[7];
    for(std::size_t i=0;i<64;++i) {
      const auto s1=rotate(e,6)^rotate(e,11)^rotate(e,25);
      const auto choose=(e&f)^(~e&g);
      const auto t1=h+s1+choose+round_constants_[i]+words[i];
      const auto s0=rotate(a,2)^rotate(a,13)^rotate(a,22);
      const auto majority=(a&b)^(a&c)^(b&c);
      const auto t2=s0+majority;
      h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    h_[0]+=a;h_[1]+=b;h_[2]+=c;h_[3]+=d;
    h_[4]+=e;h_[5]+=f;h_[6]+=g;h_[7]+=h;
  }
public:
  void update(std::string_view bytes) {
    if(finished_) throw std::runtime_error("SHA-256 digest is already finalized.");
    constexpr std::uint64_t max_bytes=(std::numeric_limits<std::uint64_t>::max)()/8;
    if(bytes.size()>max_bytes-bytes_)
      throw std::runtime_error("SHA-256 message length exceeds the standard's bound.");
    bytes_+=static_cast<std::uint64_t>(bytes.size());
    for(unsigned char ch:bytes) {
      buffer_[used_++]=ch;
      if(used_==buffer_.size()) {
        compress(buffer_.data());
        used_=0;
      }
    }
  }
  std::string hex_digest() {
    if(finished_) throw std::runtime_error("SHA-256 digest is already finalized.");
    finished_=true;
    const auto bits=bytes_*8;
    buffer_[used_++]=0x80;
    if(used_>56) {
      while(used_<64)buffer_[used_++]=0;
      compress(buffer_.data());
      used_=0;
    }
    while(used_<56)buffer_[used_++]=0;
    for(unsigned i=0;i<8;++i)
      buffer_[56+i]=static_cast<std::uint8_t>(bits>>(56-8*i));
    compress(buffer_.data());
    constexpr char hex[]="0123456789abcdef";
    std::string out;
    out.reserve(64);
    for(auto v:h_)for(int shift=28;shift>=0;shift-=4)
      out.push_back(hex[(v>>shift)&0xf]);
    return out;
  }
};
inline std::string se_hash_sha256(const std::string& text) {
  SESha256 hasher;
  hasher.update(text);
  return hasher.hex_digest();
}
inline std::string se_hash_file_sha256(const std::string& name) {
  std::ifstream in(name,std::ios::binary);
  if(!in)throw std::runtime_error("Could not read SHA-256 input file: "+name);
  SESha256 hasher;
  std::array<char,8192> buffer{};
  while(in) {
    in.read(buffer.data(),static_cast<std::streamsize>(buffer.size()));
    const auto length=in.gcount();
    if(length>0)hasher.update(std::string_view(buffer.data(),static_cast<std::size_t>(length)));
  }
  if(!in.eof())throw std::runtime_error("Could not finish reading SHA-256 input: "+name);
  return hasher.hex_digest();
}
inline std::string se_hashlib_sha256(const std::string& text) {
  return se_hash_sha256(text);
}
inline std::string se_hashlib_file_sha256(const std::string& name) {
  return se_hash_file_sha256(name);
}

// RFC 2104 HMAC construction using the SHA-256 implementation above.
inline std::string se_sha256_unhex_digest(std::string_view hex) {
  if(hex.size()!=64)throw std::runtime_error("Unexpected SHA-256 digest size.");
  std::string raw;
  raw.reserve(32);
  for(std::size_t i=0;i<hex.size();i+=2) {
    auto nibble=[](char ch)->unsigned {
      if(ch>='0'&&ch<='9')return unsigned(ch-'0');
      if(ch>='a'&&ch<='f')return unsigned(ch-'a'+10);
      throw std::runtime_error("Invalid SHA-256 internal digest.");
    };
    raw.push_back(static_cast<char>((nibble(hex[i])<<4)|nibble(hex[i+1])));
  }
  return raw;
}
inline std::string se_hashlib_hmac_sha256(std::string key,const std::string& text) {
  if(key.size()>64)key=se_sha256_unhex_digest(se_hash_sha256(key));
  key.resize(64,'\0');
  auto inner_pad=key,outer_pad=key;
  for(std::size_t i=0;i<64;++i) {
    inner_pad[i]=static_cast<char>(static_cast<unsigned char>(key[i])^0x36u);
    outer_pad[i]=static_cast<char>(static_cast<unsigned char>(key[i])^0x5cu);
  }
  SESha256 inner;
  inner.update(inner_pad);
  inner.update(text);
  const auto inner_bytes=se_sha256_unhex_digest(inner.hex_digest());
  SESha256 outer;
  outer.update(outer_pad);
  outer.update(inner_bytes);
  return outer.hex_digest();
}
inline std::string se_crypto_sha256(const std::string& text) {
  return se_hash_sha256(text);
}
inline std::string se_crypto_hmac_sha256(const std::string& key,const std::string& text) {
  return se_hashlib_hmac_sha256(key,text);
}
