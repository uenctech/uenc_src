


















#ifndef MetaString_h
#define MetaString_h

#include "Inline.h"
#include "Indexes.h"
#include "MetaRandom.h"
#include "Log.h"

namespace andrivet { namespace ADVobfuscator {

  

  template<int N, char Key, typename Indexes>
  struct MetaString;

  
  

  template<char K, int... I>
  struct MetaString<0, K, Indexes<I...>>
  {
    
    constexpr ALWAYS_INLINE MetaString(const char* str)
    : key_{K}, buffer_ {encrypt(str[I], K)...} { }

    
    inline const char* decrypt()
    {
      for(size_t i = 0; i < sizeof...(I); ++i)
        buffer_[i] = decrypt(buffer_[i]);
      buffer_[sizeof...(I)] = 0;
      LOG("--- Implementation #" << 0 << " with key 0x" << hex(key_));
      return const_cast<const char*>(buffer_);
    }

  private:
    
    constexpr char key() const { return key_; }
    constexpr char ALWAYS_INLINE encrypt(char c, int k) const { return c ^ k; }
    constexpr char decrypt(char c) const { return encrypt(c, key()); }

    volatile int key_; 
    volatile char buffer_[sizeof...(I) + 1]; 
  };

  
  

  template<char K, int... I>
  struct MetaString<1, K, Indexes<I...>>
  {
    
    constexpr ALWAYS_INLINE MetaString(const char* str)
    : key_(K), buffer_ {encrypt(str[I], I)...} { }

    
    inline const char* decrypt()
    {
      for(size_t i = 0; i < sizeof...(I); ++i)
        buffer_[i] = decrypt(buffer_[i], i);
      buffer_[sizeof...(I)] = 0;
      LOG("--- Implementation #" << 1 << " with key 0x" << hex(key_));
      return const_cast<const char*>(buffer_);
    }

  private:
    
    constexpr char key(size_t position) const { return static_cast<char>(key_ + position); }
    constexpr char ALWAYS_INLINE encrypt(char c, size_t position) const { return c ^ key(position); }
    constexpr char decrypt(char c, size_t position) const { return encrypt(c, position); }

    volatile int key_; 
    volatile char buffer_[sizeof...(I) + 1]; 
  };

  
  

  template<char K, int... I>
  struct MetaString<2, K, Indexes<I...>>
  {
    
    constexpr ALWAYS_INLINE MetaString(const char* str)
    : buffer_ {encrypt(str[I])..., 0} { }

    
    inline const char* decrypt()
    {
      for(size_t i = 0; i < sizeof...(I); ++i)
        buffer_[i] = decrypt(buffer_[i]);
      LOG("--- Implementation #" << 2 << " with key 0x" << hex(K));
      return const_cast<const char*>(buffer_);
    }

  private:
    
    
    constexpr char key(char key) const { return 1 + (key % 13); }
    constexpr char ALWAYS_INLINE encrypt(char c) const { return c + key(K); }
    constexpr char decrypt(char c) const { return c - key(K); }

    
    volatile char buffer_[sizeof...(I) + 1];
  };

  
  template<int N>
  struct MetaRandomChar
  {
    
    static const char value = static_cast<char>(1 + MetaRandom<N, 0x7F - 1>::value);
  };


}}


#define DEF_OBFUSCATED(str) andrivet::ADVobfuscator::MetaString<andrivet::ADVobfuscator::MetaRandom<__COUNTER__, 3>::value, andrivet::ADVobfuscator::MetaRandomChar<__COUNTER__>::value, Make_Indexes<sizeof(str) - 1>::type>(str)

#define OBFUSCATED(str) (DEF_OBFUSCATED(str).decrypt())

#endif
