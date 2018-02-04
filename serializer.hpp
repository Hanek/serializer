/*
 * pod serializer requirements:
 * - named blocks with pod types defined by user;
 * - varibale header length 
 */ 

// g++ -Wall -Wextra `pkg-config --cflags glib-2.0` test.cpp -o test `pkg-config --libs glib-2.0`

#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <iostream>

#include <fstream>

#ifndef _SERIALIZER_H
#define _SERIALIZER_H  



/* TODO failed malloc handling  */

class serializer
{
/*  
 *                        block structure
 * 
 *    |_|_|_|_|0|_|_|_|_|_|_|_|_|_|_|_|_|_|_|0|_|_|_|_|_|_|_|_|
 *    | dev_id  |  len  |       |             |               |
 *    | cstring | size_t|  int  |   cstring   |     float     |
 *    |s i g n a t u r e|          b l o c k   b o d y        |
 *
 *    serializer used to accumulate messages as blocks
 *    each block started with sign_block() with dev_id of variable length, 
 *    and finalized with finalize()   
 */

private:
  /* whole message length */
  int    message_len_;
  /* dynamically allocated buffer */
  char*  buf_;
    
  /* current position */
  char*  pos_;
  /* begininng of current block */
  char*  beg_;
  /* current block length */
  int block_len_;
    
  /* size of buffer */
  size_t size_;

  /* length of dev_id header */
  int hlen_;
    
  bool out_of_mem()
  {
    /* realloc preserving old data */  
    size_ *= 2;
    size_t shift1 = pos_ - buf_;
    size_t shift2 = beg_ - buf_;
    char* buf_new;
    buf_new = (char*)malloc(size_);
    if(!buf_new)
    {
      malloc_failed_stderr(__func__, errno);
      return false;
    }
    memset(buf_new, 0x00, size_);
    memcpy(buf_new, buf_, size_/2);
    free(buf_);
    buf_ = buf_new;
    pos_ = buf_ + shift1;
    beg_ = buf_ + shift2;
            
    std::cout << "out_of_mem: " << size_ << std::endl;
    return true;
  }
  
  
  void malloc_failed_stderr(const char* func, int err)
  {
    char buf[128] = {0};
    sprintf(buf, "[%s] malloc failed: %s\n", func, strerror(err)); 
    fwrite(buf, 1, strlen(buf), stderr);
  }

  
public:
  serializer(size_t size): 
  size_(size),
  hlen_(0)
  {  
    buf_  = (char*)malloc(size_);
    if(!buf_)
    { 
      malloc_failed_stderr(__func__, errno);
      return;
    }
    memset(buf_, 0x00, size_);
    message_len_ = 0;
    pos_         = buf_ + hlen_; 
    beg_         = buf_;
    block_len_   = 0;
  }
  
  ~serializer() { free(buf_); }
  bool empty() { return (0 == size_) ? 1 : 0; }
  size_t get_size() { return size_; }
  size_t length() { return message_len_; }
  size_t get_hlen() { return hlen_; }
  const char* buffer_fetch() { return buf_; }
  
  /* copy data to serializer */
  void buffer_update(const char* bufin, size_t sizein)
  {
    while(size_ <= sizein)
    { out_of_mem(); }
    memcpy(buf_, bufin, sizein);
    message_len_ = (int)sizein;
  }
  
  void clear()
  {
    reset();
    memset(buf_, 0x00, size_); 
    message_len_ = 0;
  }
  
  void reset()
  {
    hlen_      = 0;
    pos_       = buf_ + hlen_; 
    beg_       = buf_;
    block_len_ = 0;
  }
  
  /* called prior to device serialization */ 
  void sign_block(const char* id)
  {
    if(!id)
    {
      hlen_ = 1;
      *beg_ = 0x00;
      while(pos_ - buf_ + hlen_ + sizeof(hlen_) >= size_)
      { out_of_mem(); }
    }
    else
    {
      hlen_ = strlen(id) + 1;
      while(pos_ - buf_ + hlen_ + sizeof(hlen_) >= size_)
      { out_of_mem(); }
      memcpy(beg_, id, hlen_);
      *(beg_ + hlen_) = 0x00;
    }
    
    pos_ += hlen_ + sizeof(hlen_);
  }
  
  /* called when device serializati on is done */
  void finalize_block()
  {
    block_len_ = (pos_ - beg_) - hlen_ - sizeof(block_len_);
    message_len_ += (pos_ - beg_);
    memcpy(beg_ + hlen_, &block_len_, sizeof(block_len_));
    beg_ = pos_;
  }
  
  
  
  /* read block one by one while the end is not reached */
  bool read_block(char* id)
  {    
    strcpy(id, pos_);
    hlen_ = strlen(id);
    
    while(pos_ - buf_ + hlen_ + sizeof(hlen_) >= size_)
    { out_of_mem(); }
    
    if(message_len_ <= pos_ - beg_)
    { return false; }
        
    pos_ = beg_ + hlen_ + sizeof(hlen_) + 1;
    memcpy(&block_len_, beg_ + hlen_ + 1, sizeof(block_len_));
    beg_ += block_len_ + hlen_ + sizeof(block_len_) + 1; 

    return true;
  }
  
  /********** serialization methods **********/
  
  void serialize_cstring(const char* str) 
  {
    if(!str) 
    {
      if(4 >= size_ - (pos_ - buf_) - 1)
      { out_of_mem(); }
      
      memcpy(pos_, "NULL", 4); 
      pos_ += 4; 
      *pos_ = 0x00; 
      pos_ += 1;
      return;
    }
    
    if(strlen(str) >= size_ - (pos_ - buf_) - 1)
    { out_of_mem(); }
    
    memcpy(pos_, str, strlen(str)); 
    pos_ += strlen(str); 
    *pos_ = 0x00; 
    pos_ += 1;
  }
  
  
  template <class T> void serialize(T var) 
  {
    if(sizeof(T) >= size_ - (pos_ - buf_))
    { out_of_mem(); }
    
    memcpy(pos_, &var, sizeof(T)); 
    pos_ += sizeof(T);
  }
  
  /********** deserialization methods **********/
  
  void deserialize_cstring(char* str)
  {
    memcpy((void*)str, pos_, strlen(pos_)); 
    pos_ += strlen(pos_) + 1;
  }
  
  
  template <class T> void deserialize(T* var)
  {
    memcpy((void*)var, pos_, sizeof(T)); 
    pos_ += sizeof(T);
  }
  
  void dump()
  {
    std::ofstream file("buf.bin", std::ios::out | std::ios::binary);
    file.write(buf_, size_);
    file.close();
  }
};
#endif

