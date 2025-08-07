#include "kdbreader.hpp"
#include "qfile.hpp"
//#include <limits>
#include <fstream>
#include "readstring.hpp"

#define DEBUG_MODE 1


KDBFileReader::KDBFileReader(const std::string &path, const std::string& sympath) 
: path_(path), sympath_(sympath),readed_size_(0),total_size_(0),finished_(false)//, nested_(nullptr)
{
    this->istream_ = std::make_shared<filestream>(path);
};

KDBFileReader::~KDBFileReader() {
//    fclose(this->fp_);
}
void KDBFileReader::set_istream(std::shared_ptr<inputstream> istream)
{
    this->istream_ = istream;
}


int KDBFileReader::read_meta(size_t base_offset) {
#if DEBUG_MODE 
    std::cout<<"info,read_meta:"<<this->path_<<",base_offset:"<<base_offset<<std::endl;
#endif    
    // this->fp_ = fopen(this->path_.c_str(), "rb");
    // this->file_len_ = get_file_length(this->fp_);
    this->file_len_ = this->istream_->get_length();

    // fseek(fp_, base_offset, SEEK_SET);    
    this->istream_->fseek1(base_offset, SEEK_SET);
    // fread(this->header, 1, KDB_HEADER_BYTES, fp_);
    this->istream_->fread1(this->header,1, KDB_HEADER_BYTES);
    //readlist<char>(fp_, 0, KDB_HEADER_BYTES, header);
    //std::cout<<"info,header:"<<strncmp(header,"\xfe\x20")<<","<<strncmp(header,"\xff\01")<<","<<strncmp(header,"\xFD\00")<<std::endl;   //7       
    //std::cout<<"info,header:"<<(std::string(header.data(),2)==std::string("\xfe\x20",2))<<","<<(std::string(header.data(),2)==std::string("\xff\01",2))<<std::endl;   //7   

    char datatype;        
    //size_t read = fread(&datatype, DTYPE_BYTES, 1, fp_);
    size_t read = this->istream_->fread1(&datatype, DTYPE_BYTES, 1);
    //char datatype = readatom<char>(fp_, KDB_HEADER_BYTES, DTYPE_BYTES);
    std::cout<<"info, datatype0:"<<(int)datatype<<std::endl;   //7
    this->dtype_ = (int)datatype;

    std::cout << "debug,position1:"<<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[0])) << " " <<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[1]))<<std::endl;
    if(strncmp(header,"kx",KDB_HEADER_BYTES)==0) {  //.data()
        //std::cout<<"error,unsupported kx"<<std::endl;  
        size_t MAGIC_BYTES = 8;
        std::vector<char> header(MAGIC_BYTES, '\0');
        //fseek(fp_, 0, SEEK_SET);
        //const auto read = fread(header.data(), 1, header.size(), fp_);
        this->istream_->fseek1(0, SEEK_SET);
        const auto read = this->istream_->fread1(header.data(), 1, header.size());
        const std::string magic{header.cbegin(), header.cend()};
        if(magic == "kxzipped") {
#if DEBUG_MODE 
            std::cout<<"debug,kxzipped"<<std::endl;
#endif
            std::shared_ptr<filestream> fstrem = std::dynamic_pointer_cast<filestream>(this->istream_);
            assert(fstrem);
            kdb::BinFile binfile = kdb::BinFile(fstrem->fp_);

            //std::vector<byte> buffer;             
            std::shared_ptr<bufferstream> buffer_stream = std::make_shared<bufferstream>();
            binfile.inflateBody(buffer_stream->buffer_);
            //std::cout<<"info,inflateBody:"<<buffer_stream->buffer_.size()<<std::endl;

            //std::string tmp_file("/home/yky/duckdb_dev/testdata/bp10_1");
            //std::ofstream outfile(tmp_file, std::ios::binary);
            //outfile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            this->set_istream(buffer_stream);
            return this->read_meta(0);
        }
        else {
            std::cout<<"error,unsupported kx"<<std::endl;
        }
    }
    if((int)datatype<20) {
        std::cout << "debug,position2:"<<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[0])) << " " <<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[1]))<<std::endl;
        if(strncmp(header,"\xfe\x20",KDB_HEADER_BYTES)==0) {   //list
#if DEBUG_MODE         
            std::cout<<"info,header:FE20,dtype:"<<(int)datatype<<std::endl;
#endif            
            this->offset_ = 16+base_offset;
            this->file_byte_size_ = this->file_len_-this->offset_;
            //read0<T>((int)datatype, fp_, offset, byte_size, item_start, item_read, out);
            return 0;
        }
        else if(strncmp(header,"\xff\01",KDB_HEADER_BYTES)==0) {  //atoms
#if DEBUG_MODE         
            std::cout<<"info,header:FF01,dtype:"<<(int)datatype<<std::endl;
#endif            
            this->offset_ = 3+base_offset;
            this->file_byte_size_ = this->file_len_-this->offset_;
            //read0<T>((int)datatype, fp_, offset, byte_size, item_start, item_read, out);
            return 0;
        }
    }
    std::cout << "debug,position3:"<<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[0])) << " " <<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[1]))<<std::endl;
    if((strncmp(header,"\xFD\00",KDB_HEADER_BYTES)==0)&&((int)datatype==4)) {
#if DEBUG_MODE     
        std::cout<<"info,header:FD00,dtype:"<<(int)datatype<<std::endl;
#endif                
        this->offset_ = 16+base_offset;
        this->file_byte_size_ = this->file_len_-this->offset_;
        return 0;

    }
    std::cout << "debug,position4:"<<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[0])) << " " <<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[1]))<<std::endl;
    if((strncmp(header,"\xFD\00",KDB_HEADER_BYTES)==0)&&((int)datatype>=20)&&((int)datatype<=76)) {   //enum
#if DEBUG_MODE     
        std::cout<<"info,header:FD00,dtype:"<<(int)datatype<<",";
        std::cout<<"This is an extension header following kdb::Parser::ExtListHeader."<<std::endl;
#endif        
        this->offset_ = base_offset+16;
        this->file_byte_size_ = this->file_len_ - this->offset_;
        //std::vector<long> idx; 
        //readlist1<long>(fp_, base_offset+16, file_len-(base_offset+16), item_start, item_read, idx);
        //std::cout<<"info,enum idx:"<<idx.size()<<","<<idx[0]<<","<<idx[10]<<std::endl;

        if(this->sym_vec_.empty()) {
#if DEBUG_MODE             
            std::cout<<"info,read sym file..."<<sympath_<<std::endl;  
#endif
            KDBFileReader sym_reader = KDBFileReader(this->sympath_, "");
            sym_reader.read_meta();
            sym_reader.read<std::string>(0, 10000000, this->sym_vec_);   //read all,std::numeric_limits<size_t>::max()
#if DEBUG_MODE             
            std::cout<<"info,read sym file completed:"<<this->sym_vec_.size()<<" ------------------------"<<std::endl; 
#endif            
            assert(this->sym_vec_.size()>0);
        }

    }
    std::cout << "debug,position5:"<<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[0])) << " " <<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[1]))<<std::endl;
    if((strncmp(header,"\xFD\01",KDB_HEADER_BYTES)==0)&&((int)datatype==77)) {   //anymap
#if DEBUG_MODE  
        std::cout<<"info,header:FD01,dtype:"<<(int)datatype<<",basestrfile:anymap"<<std::endl;
#endif
        this->offset_ = base_offset+16;
        this->file_byte_size_ = this->file_len_ - this->offset_;

/*
        std::vector<long> nested_idx_vec;
        this->read<long>(0,4,nested_idx_vec);
        std::cout<<"debug,nested_idx_vec:"<<nested_idx_vec.size()<<std::endl;
        for(size_t i=0;i<nested_idx_vec.size();i++)
            std::cout<<nested_idx_vec[i]<<std::endl;

#if DEBUG_MODE          
        std::cout<<"info,read # file..."<<std::endl;  
#endif
        this->nested_ = std::make_shared<KDBFileReader>(this->path_+"#", "");
        return this->nested_->read_meta(); 
*/

        if(!this->str_vec_.empty())
            return 0;

        int num_strings = 0;
        char** string_array = NULL;
        // 调用函数读取字符串数组
        string_array = readStringArray(this->path_.c_str(), &num_strings);
        if (string_array) {
#if DEBUG_MODE             
            printf("info,Read %d strings:\n", num_strings);
#endif
            for (int i = 0; i < num_strings; ++i) {
                //printf("[%d]: %s\n", i, string_array[i]);
                this->str_vec_.push_back(string_array[i]);
            }
            // 释放内存
            freeStringArray(string_array, num_strings);
        } else {
            printf("Failed to read string array. Please ensure the files exist and are correctly formatted.\n");
        }

    }    
    if(((int)datatype>77)&&((int)datatype<97)) {
        std::cout<<"error,not implemented"<<std::endl;
        assert(0);
    }

    std::cout << "debug,position6:"<<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[0])) << " " <<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[1]))<<std::endl;
    std::cout<<"debug,KDB_HEADER_BYTES:"<<KDB_HEADER_BYTES<<std::endl;
    int res = strncmp(header,"\xfd\x20",KDB_HEADER_BYTES);
    int cres = (res == 0);

    std::cout << "res 的地址: " << &res << ", res 的值: " << res << ", res 的十六进制值: " << std::hex << res << std::endl;
    std::cout<<"debug strncmp: "<<std::dec << res<<", isEqual: "<< cres <<std::endl;
    std::cout << "datatype: "<<(int)datatype<<std::endl;
    if(cres) {   
        std::cout << "进入了 if (res == 0) 分支" << std::endl;
        std::cout<<"infonew,header:FD20,dtype:"<<(int)datatype<<",ExtListHeader"<<std::endl;
        return this->read_meta(0x1000-16);
    }
        
}
