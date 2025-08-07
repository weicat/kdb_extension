#include "kdbreader.hpp"
#include "qfile.hpp"
//#include <limits>
#include <fstream>
#include "readstring.hpp"

#define DEBUG_MODE 1

void print_hex(const char* str, int n) {
    for (int i = 0; i < n ; i++) {
        printf("%02x ", (unsigned char)str[i]); // 打印为两位十六进制数
    }
    printf("\n");
}

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
    this->file_len_ = this->istream_->get_length();

    this->istream_->fseek1(base_offset, SEEK_SET);
    this->istream_->fread1(this->header,1, KDB_HEADER_BYTES);

    char datatype;        
    size_t read = this->istream_->fread1(&datatype, DTYPE_BYTES, 1);
    std::cout<<"info, datatype0:"<<(int)datatype<<std::endl;   //7
    this->dtype_ = (int)datatype;

    std::cout << "debug,position1:"<<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[0])) << " " <<std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(header[1]))<<std::endl;
    if(strncmp(header,"kx",KDB_HEADER_BYTES)==0) {  //.data()
        size_t MAGIC_BYTES = 8;
        std::vector<char> header(MAGIC_BYTES, '\0');
        this->istream_->fseek1(0, SEEK_SET);
        const auto read = this->istream_->fread1(header.data(), 1, header.size());
        const std::string magic{header.cbegin(), header.cend()};
        if(magic == "kxzipped") {
            std::shared_ptr<filestream> fstrem = std::dynamic_pointer_cast<filestream>(this->istream_);
            assert(fstrem);
            kdb::BinFile binfile = kdb::BinFile(fstrem->fp_);

            std::shared_ptr<bufferstream> buffer_stream = std::make_shared<bufferstream>();
            binfile.inflateBody(buffer_stream->buffer_);
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
            this->offset_ = 16+base_offset;
            this->file_byte_size_ = this->file_len_-this->offset_;
            //read0<T>((int)datatype, fp_, offset, byte_size, item_start, item_read, out);
            return 0;
        }
        else if(strncmp(header,"\xff\x01",KDB_HEADER_BYTES)==0) {  //atoms
            this->offset_ = 3+base_offset;
            this->file_byte_size_ = this->file_len_-this->offset_;
            //read0<T>((int)datatype, fp_, offset, byte_size, item_start, item_read, out);
            return 0;
        }
    }
    printf("HEADER: ");
    print_hex(header, 2);
    std::cout<<std::dec<<(int)datatype<<std::endl;
    
    if((strncmp(header,"\xFD\x00",KDB_HEADER_BYTES)==0)&&((int)datatype==4)) {
        this->offset_ = 16+base_offset;
        this->file_byte_size_ = this->file_len_-this->offset_;
        return 0;

    }else if((strncmp(header,"\xFD\x00",KDB_HEADER_BYTES)==0)&&((int)datatype>=20)&&((int)datatype<=76)) {   //enum
        this->offset_ = base_offset+16;
        this->file_byte_size_ = this->file_len_ - this->offset_;
        //std::vector<long> idx; 
        //readlist1<long>(fp_, base_offset+16, file_len-(base_offset+16), item_start, item_read, idx);
        //std::cout<<"info,enum idx:"<<idx.size()<<","<<idx[0]<<","<<idx[10]<<std::endl;

        if(this->sym_vec_.empty()) {
            KDBFileReader sym_reader = KDBFileReader(this->sympath_, "");
            sym_reader.read_meta();
            sym_reader.read<std::string>(0, 10000000, this->sym_vec_);   //read all,std::numeric_limits<size_t>::max()
            assert(this->sym_vec_.size()>0);
        }

    }else if((strncmp(header,"\xFD\x01",KDB_HEADER_BYTES)==0)&&((int)datatype==77)) {   //anymap
        this->offset_ = base_offset+16;
        this->file_byte_size_ = this->file_len_ - this->offset_;

        if(!this->str_vec_.empty())
            return 0;

        int num_strings = 0;
        char** string_array = NULL;
        // 调用函数读取字符串数组
        string_array = readStringArray(this->path_.c_str(), &num_strings);
        if (string_array) {
            for (int i = 0; i < num_strings; ++i) {
                //printf("[%d]: %s\n", i, string_array[i]);
                this->str_vec_.push_back(string_array[i]);
            }
            // 释放内存
            freeStringArray(string_array, num_strings);
        } else {
            printf("Failed to read string array. Please ensure the files exist and are correctly formatted.\n");
        }

    }else if(strncmp(header,"\xfd\x20",KDB_HEADER_BYTES)) {   
        std::cout << "进入了 if (res == 0) 分支" << std::endl;
        std::cout<<"infonew,header:FD20,dtype:"<<(int)datatype<<",ExtListHeader"<<std::endl;
        return this->read_meta(0x1000-16);
    }
        
}
