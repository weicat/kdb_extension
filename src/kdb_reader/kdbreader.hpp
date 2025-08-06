#pragma once

#include <cassert>
#include <sstream>
#include <iomanip>
#include <memory> 
#include <string>
#include <algorithm>
#include "read.hpp"
//#include "handlers.hpp"
#include "utils.hpp"
#include "inputstream.hpp"

#define DEBUG_MODE  1

#define DTYPE_BYTES 1

//#define KDB_HEADER_BYTES  2
const int KDB_HEADER_BYTES = 2;

class KDBFileReader {
private:
    //FILE* fp_;
    size_t file_len_;
    std::string path_;    
    std::string sympath_;
    std::shared_ptr<inputstream> istream_;

public:    
    std::string field_name_;

private:
//public:
    char header[KDB_HEADER_BYTES];
    int dtype_;    
    size_t offset_;
    size_t file_byte_size_;

    size_t total_size_;
    size_t readed_size_;
    bool finished_;

    std::vector<std::string> sym_vec_;
    //std::shared_ptr<KDBFileReader> nested_;
    std::vector<std::string> str_vec_;

public:
    KDBFileReader(const std::string &path, const std::string& sympath);
    ~KDBFileReader();
    void set_istream(std::shared_ptr<inputstream> istream);

    int read_meta(size_t base_offset=0);

    template <typename T>
    size_t read(size_t item_start, size_t item_read, std::vector<T>& out);
    
    int get_dtype() const;
    size_t get_readed_size() const;
    void set_finished();
    bool is_finished() const;
};


template<typename T>
size_t KDBFileReader::read(size_t item_start, size_t item_read, std::vector<T>& out) 
{
    // if(nested_) {
    //     std::cout<<"info,delegate to nested:"<<std::endl;
    //     return nested_->read<T>(item_start, item_read, out);
    // }
    //std::cout<<"info,read:"<<this->offset_<<","<<this->file_byte_size_<<std::endl;

    out.clear();
    if(this->dtype_<20) {
        if(strncmp(this->header,"\xfe\x20",KDB_HEADER_BYTES)==0) {   //list
            read0<T>(this->dtype_, this->istream_, this->offset_, this->file_byte_size_, item_start, item_read, out); 
        }
        else if(strncmp(this->header,"\xff\01",KDB_HEADER_BYTES)==0) {  //atoms
            read0<T>(this->dtype_, this->istream_, this->offset_, this->file_byte_size_, 0, 0, out);
        }
        else if(strncmp(this->header,"\xfd\x00",KDB_HEADER_BYTES)==0) {   //#file
#if DEBUG_MODE         
            std::cout<<"info,read header:FD00"<<std::endl;
#endif
            read0<T>(this->dtype_, this->istream_, this->offset_, this->file_byte_size_, item_start, item_read, out); 
        }
        else {
            std::cout<<"error,read failed. unknown header type"<<std::endl;
        }
    } 
    else if((strncmp(this->header,"\xFD\01",KDB_HEADER_BYTES)==0) && (this->dtype_==77)) {
#if DEBUG_MODE  
        std::cout<<"info,read FD01,dtype:77"<<std::endl;
#endif
        //read0<T>(this->dtype_, this->istream_, this->offset_, this->file_byte_size_, item_start, item_read, out); 
        std::cout<<"error,dtype 77 mapping to strings output"<<std::endl;
        assert(0);
    }    
    else {
        std::cout<<"error,unknown header,dtype"<<std::endl;
        assert(0);
    }

    this->readed_size_ += out.size();
    return out.size();
}


template<>
inline size_t KDBFileReader::read(size_t item_start, size_t item_read, std::vector<std::string>& out) 
{
    // if(nested_)
    //     return nested_->read<std::string>(item_start, item_read, out);

    out.clear();
    if(this->dtype_<20) {
        if(strncmp(this->header,"\xfe\x20",KDB_HEADER_BYTES)==0) {   //list
            //std::cout<<"debug,read0 list"<<std::endl;
            read0<std::string>(this->dtype_, this->istream_, this->offset_, this->file_byte_size_, item_start, item_read, out);
        }
        else if(strncmp(this->header,"\xff\01",KDB_HEADER_BYTES)==0) {  //atoms
            //std::cout<<"debug,read0 atoms"<<std::endl;
            //读sym文件时，会进入atoms模式，虽然dtype==11。调用特化read0<std::string>
            read0<std::string>(this->dtype_, this->istream_, this->offset_, this->file_byte_size_, item_start, item_read, out);            
        }
    }
    else if((strncmp(header,"\xFD\00",KDB_HEADER_BYTES)==0)&&(this->dtype_>=20)&&(this->dtype_<=77)) {   //enum  
        //std::cout<<"debug,read enum"<<std::endl;
        std::vector<long> idx;         
        readlist1<long>(this->istream_, this->offset_, this->file_byte_size_, item_start, item_read, idx);
        //std::cout<<"info,enum idx:"<<idx.size()<<","<<idx[0]<<","<<idx[10]<<std::endl;
        for(long i=0;i<idx.size();i++)
            out.push_back(this->sym_vec_[idx[i]]);
    }
    else if((strncmp(this->header,"\xFD\01",KDB_HEADER_BYTES)==0) && (this->dtype_==77)) {
#if DEBUG_MODE    
        std::cout<<"info,read FD01,dtype:77"<<std::endl;
#endif
        //read0<T>(this->dtype_, this->istream_, this->offset_, this->file_byte_size_, item_start, item_read, out);
        assert(item_start<str_vec_.size());
        item_read = std::min(str_vec_.size()-item_start,item_read);
        out.insert(out.end(), str_vec_.begin()+item_start, str_vec_.begin()+item_start+item_read); 
    }   
    else {
        std::cout<<"error,unknown header,dtype"<<std::endl;
        assert(0);
    }

    this->readed_size_ += out.size();
    return out.size();
}


inline int KDBFileReader::get_dtype() const {
    // if(nested_)
    //     return nested_->get_dtype();

    return this->dtype_;
}
inline size_t KDBFileReader::get_readed_size() const {
    // if(nested_)
    //     return nested_->get_readed_size();

    return this->readed_size_;
}
inline void KDBFileReader::set_finished() {
    // if(nested_)
    //     return nested_->set_finished();

    this->finished_ = true;
    this->total_size_ = this->readed_size_;
    return;
}
inline bool KDBFileReader::is_finished() const {
    // if(nested_)
    //     return nested_->is_finished();

    return this->finished_;
}
