#include <clocale>
#include <pthread.h>
#include <typeinfo>
#include <iostream>
#include <string>
#include <sstream>
#include <queue>
#include <vector>
#include <glib.h>

#include "serializer.hpp"
  

void linear_serializer_test()
{ 
  int n = 2;
  char p[4096] = {0};
  char str[4096];
  float f = 3.141592;
      
  serializer is(4), os(4);
  
  for(int i = 0; i < n; i++)
  {
    sprintf(str, "cstring: #%d", i<<2);
    is.sign_block(p);
    is.serialize<int>(4096 + i);
    is.serialize<char>('a' + i);
    is.serialize_cstring(NULL);
    is.serialize<char>('A' + i);
    is.serialize<float>(f + i);
    is.finalize_block();
  }
  
  gchar* strEncoded = g_base64_encode((const guchar*)is.buffer_fetch(), is.length());
  is.reset();

//   std::cout << "input buffer length: " << is.length() << std::endl;  
  
  is.dump();
  
  gsize lenDecoded;
  guchar* strDecoded = g_base64_decode(strEncoded, &lenDecoded);
  
  os.buffer_update((const char*)strDecoded, lenDecoded);  
  g_free(strEncoded);
  g_free(strDecoded);
  
  int ii = 0;
  char cc = 0;
  char c2 = 0;
  char pp[64];
  float ff = 0;
  
  memset(pp, 0x00, sizeof(pp));
  memset(p, 0x00, sizeof(p));
  
  for(int i = 0; i < n; i++)
  {
    os.read_block(p);
    os.deserialize<int>(&ii);
    os.deserialize<char>(&cc);
    os.deserialize_cstring(pp);
    os.deserialize<char>(&c2);
    os.deserialize<float>(&ff);
    
    std::cout << "id: " << p << std::endl;
    std::cout << ii << std::endl;
    std::cout << cc << std::endl;
    std::cout << pp << std::endl;
    std::cout << c2 << std::endl;
    std::cout << ff << std::endl;
    std::cout << "--------------------------------------------" << std::endl;
    memset(pp, 0x00, sizeof(pp));
  }
}



void linear_std_stream_test(int n)
{ 
  char h[16];
  strcpy(h, "h4bt");
  char c = 'a';
  char p[64];
  strcpy(p, "test_string");
  float f = 3.141592;

  
  std::stringstream os;
   
  for(int i = 0; i < n; i++)
  {
    os << h << " "
       << 0xffff + i << " " 
       << (char)(c + i) << " "
       << p << " "
       << f << " ";
       
//        std::cout << h << "\t";
//        std::cout << i + 0xffff << "\t";
//        std::cout << (char)(c + i) << "\t";
//        std::cout << p << "\t";
//        std::cout << f << std::endl;
  }
  
  
  std::cout << "std stream length: " << os.str().length() << std::endl;
  
 
  std::string hh;
  int ii = 0;
  char cc = 0;
  std::string pp;
  float ff = 0;
  
  
  for(int i = 0; i < n; i ++)
  {
    pp.clear();
    os >> hh
       >> ii 
       >> cc
       >> pp 
       >> ff;
       
//        std::cout << hh << "\t";
//        std::cout << ii << "\t";
//        std::cout << cc << "\t";
//        std::cout << pp << "\t";
//        std::cout << ff << std::endl;
  } 
}


void test_std_queue()
{
  int lim = 1000;
  std::queue<std::string> qu;
  
  std::string p = "test string";
  for(int n = 0; n < lim; n++)
  {
    p[3] = 0x30 + n;
    qu.push(p);
  }
 
    
  for(int n = 0; n < lim; n++)
  {
    std::string pp;
    pp = qu.front();
    qu.pop();
//     std::cout << "node: " << pp << std::endl;
  }
}
// 100000000
// 132000000 serializer
// 139031070 std_stream  
 
int main()
{
  linear_serializer_test();
//   linear_std_stream_test(4000000);  
  
  return 0;
}