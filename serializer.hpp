/*
 * pod serializer requirements:
 * - allocate memory when needed;
 * - named blocks with pod types defined by user; 
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
 *    |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|0|_|_|_|_|_|_|_|_|
 *    | dev_id  |  len  |       |             |               |
 *    | cstring | size_t|  int  |   cstring   |     float     |
 *    |s i g n a t u r e|          b l o c k   b o d y        |
 *
 *    serializer used to accumulate messages as blocks
 *    each block finalized on sign_block() call with dev_id
 *    of predefined length, which has to be the same on the other end.   
 */

private:
  /* whole message length */
  int    message_len_;
  /* dynamically allocated buffer */
  char*  buf_;
  /* true if no messages being signed */
  bool   head_;
    
  /* current position */
  char*  pos_;
  /* begininng of current block */
  char*  beg_;
  /* current block length */
  int block_len_;
    
  /* size of buffer */
  size_t size_;

  /* length of dev_id header */
  const int hlen_;
    
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
  
  bool realloc(size_t sizein)
  {
    /* plain realloc  */
    size_ = sizein;
    char* buf_new;
    buf_new = (char*)malloc(size_);
    if(!buf_new)
    {
      malloc_failed_stderr(__func__, errno);
      return false;
    }
    free(buf_);
    buf_ = buf_new;
      
    memset(buf_, 0x00, size_);                                     
    pos_       = buf_ + hlen_ + sizeof(block_len_); 
    beg_       = buf_;
    block_len_ = 0;
    head_      = true;     
    std::cout << "realloc: " << size_ << std::endl;
    return true;
  }
  
  void malloc_failed_stderr(const char* func, int err)
  {
    char buf[128] = {0};
    sprintf(buf, "[%s] malloc failed: %s\n", func, strerror(err)); 
    fwrite(buf, 1, strlen(buf), stderr);
  }

public:
  /* header_len [0 ... 64] - header length */ 
  serializer(size_t header_len): 
  size_(1024),
  hlen_(header_len > 64 ? 64 : header_len)
  {  
    buf_  = (char*)malloc(size_);
    if(!buf_)
    { 
      malloc_failed_stderr(__func__, errno);
      return;
    }
    memset(buf_, 0x00, size_);
    message_len_ = 0;
    pos_         = buf_ + hlen_ + sizeof(block_len_); 
    beg_         = buf_;
    block_len_   = 0;
    head_        = true;
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
    if(size_ <= sizein)
    { realloc(sizein); }
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
    pos_       = buf_ + hlen_ + sizeof(block_len_); 
    beg_       = buf_;
    block_len_ = 0;
    head_      = true;
  }
  
  /* 
   * called when device serialization is done
   * id has to be a cstring, will be truncated to hlen too long.. 
   */
  void sign_block(const char* id)
  {
    block_len_ = (pos_ - beg_) - hlen_ - sizeof(block_len_);
    message_len_ += (pos_ - beg_);
    
    if(id && strlen(id) > 0)
    { memcpy(beg_, id, hlen_); }
    else
    { if(hlen_ > 0) { memset(beg_, ' ', hlen_); } }
    
    memcpy(beg_ + hlen_, &block_len_, sizeof(block_len_));
    beg_ = pos_;
    if(size_ - (pos_ - buf_) <= (hlen_ + sizeof(block_len_)))
    { out_of_mem(); }
    pos_ += hlen_ + sizeof(block_len_);
  }
  
  /* read block one by one while the end is not reached */
  bool read_block(char* id)
  {
    if(message_len_ <= pos_ - beg_)
    { return false; }
    if(!head_)
    { beg_ += block_len_ + hlen_ + sizeof(block_len_); }
    
    if(0x00 == *beg_) { return false; }
    pos_ = beg_ + hlen_ + sizeof(block_len_);
    memcpy(id, beg_, hlen_);
    memcpy(&block_len_, beg_ + hlen_, sizeof(block_len_));
    head_ = false;
    
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

