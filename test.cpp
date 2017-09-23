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


// g++ -Wall -Wextra `pkg-config --cflags glib-2.0` test.cpp -o test `pkg-config --libs glib-2.0`


void linear_serializer_test(int hlen, int npasses)
{ 
  char c = 'a';
  char p[64];
  strcpy(p, "test string");
  float f = 3.141592;
   
  serializer is(hlen);
  serializer os(hlen);
 
  for(int i = 0; i < npasses; i++)
  {
    char block[hlen] = {0};
    is.serialize<int>(i + 0xffff);
    is.serialize<char>(c + i);
    is.serialize_cstring(p);
    is.serialize<float>(f); 
    if(hlen > 0)
    {
      strcpy(block, "bl");
      block[strlen(block)] = 0x30 + i;
      is.sign_block(block);
    }
    else
    { is.sign_block(NULL); }
    
    
    std::cout << "block: " << block << ":\t";
    std::cout << i + 0xffff << "\t";
    std::cout << (char)(c + i) << "\t";
    std::cout << p << "\t";
    std::cout << f << std::endl;  

  }
  
  gchar* strEncoded = g_base64_encode((const guchar*)is.buffer_fetch(), is.length());
  is.reset();
  std::cout << "input buffer length: " << is.length() << std::endl;  
  
  is.dump();
  
  gsize lenDecoded;
  guchar* strDecoded = g_base64_decode(strEncoded, &lenDecoded);
  
  os.buffer_update((const char*)strDecoded, lenDecoded);  
  g_free(strEncoded);
  g_free(strDecoded);
  
  int ii = 0;
  char cc = 0;
  char pp[64];
  float ff = 0;   
  
  for(int i = 0; i < npasses; i ++)
  {
    memset(pp, 0x00, sizeof(pp));
    memset(p, 0x00, sizeof(p));
    os.read_block(p);
    os.deserialize<int>(&ii);
    os.deserialize<char>(&cc);
    os.deserialize_cstring(pp);
    os.deserialize<float>(&ff);
    
    std::cout << "block: " << p << ":\t";
    std::cout << ii << "\t";
    std::cout << cc << "\t";
    std::cout << pp << "\t";
    std::cout << ff << std::endl;  
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
  linear_serializer_test(64, 5);
//   linear_std_stream_test(4000000);  
  
  return 0;
}