#include <iostream>

#include <assert.h>

#include <string.h>

#include <math.h>

#include <sys/types.h>

#include <unistd.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <map>

#include <vector>

#include <fstream>

#include <algorithm>

#include <bits/stdc++.h>

using namespace std;

#define DISK_SIZE 256

// ============================================================================
void decToBinary(int n, char & c) {
    // array to store binary number
    int binaryNum[8];

    // counter for binary array
    int i = 0;
    while (n > 0) {
        // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }

    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--) {
        if (binaryNum[j] == 1)
            c = c | 1u << j;
    }
}

// ============================================================================

class FsFile {
    int file_size;
    int block_in_use;
    int index_block;
    int block_size;

    public:
        FsFile(int _block_size) {
            file_size = 0;
            block_in_use = 0;
            block_size = _block_size;
            index_block = -1;
        }
    void set_index_block(int free_index) {
        this -> index_block = free_index;
    }
    void increase_block_in_use() {
        block_in_use++;
    }
    void increase_file_size() {
        file_size++;
    }
    int get_block_in_use() {
        return block_in_use;
    }

    int get_file_size() {
        return file_size;
    }
    int get_index_block() {
        return index_block;
    }
    void print() {
        cout << "index block=" << index_block << "\tfilesize=" << file_size << "\tblock in use=" << block_in_use << endl;
    }
};

// ============================================================================

class FileDescriptor {
    string file_name;
    FsFile * fs_file;
    bool inUse;

    public:

        FileDescriptor(string FileName, FsFile * fsi) {
            file_name = FileName;
            fs_file = fsi;
            inUse = true;
        }

    string getFileName() {
        return file_name;
    }

    bool get_inUse() {
        return inUse;
    }
    void set_inUse(bool set) {
        inUse = set;
    }

    FsFile * get_fs() {
        return fs_file;
    }

};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"

// ============================================================================

class fsDisk {
    FILE * sim_disk_fd;
    bool is_formated;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.
    int BitVectorSize;
    int * BitVector;

    // (5) MainDir --
    // Structure that links the file name to its FsFile
    std::vector < FileDescriptor > MainDir;
    // (6) OpenFileDescriptors --
    //  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    std::map < string, int > OpenFileDescriptors;
    int block_size;
    priority_queue <int, vector<int>, greater<int> > avalible_fd;
    bool* is_index_block;
    int real_zero_index;
    public:
        fsDisk() {
            sim_disk_fd = fopen(DISK_SIM_FILE, "w+");
            assert(sim_disk_fd);

            for (int i = 0; i < DISK_SIZE; i++) {
                int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
                ret_val = fwrite("\0", 1, 1, sim_disk_fd);
                assert(ret_val == 1);
            }

            fflush(sim_disk_fd);
            is_formated = false;
            real_zero_index=-1;
        }
        ~fsDisk() {
            for (int i = 0; i < MainDir.size(); i++)
                free(MainDir[i].get_fs());
            fclose(sim_disk_fd);
            free(BitVector);
            free(is_index_block);
        }
    void listAll() {
        if(!is_formated){
            cout<<"disk not formatted"<<endl;
            return;
        }
        int i = 0;
        for (; i < MainDir.size(); i++)
            cout << "index: " << i << ": FileName: " << MainDir[i].getFileName() << " , isInUse: " << MainDir[i].get_inUse() << endl;

        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++) {
            cout << "(";
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread( & bufy, 1, 1, sim_disk_fd);
            if(is_index_block[i/block_size] && ((int) bufy>0 || i==real_zero_index))
                cout << (int) bufy;
            else
                cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }
    /**
     * fsFormat format the disk with the given block size
     * 
     * @param blockSize the size of single block in the disk
     * 
     */
    void fsFormat(int blockSize = 4) {
        for (int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        this -> block_size = blockSize;
        BitVectorSize = DISK_SIZE / block_size;
        if (is_formated){
            free(BitVector);
            free(is_index_block);
        }
        BitVector = (int * ) malloc(sizeof(int) * BitVectorSize);
        if (BitVector == NULL) {
            perror("malloc");
            printf("not formatted\n");
            return;
        }
        for (int i = 0; i < BitVectorSize; i++)
            BitVector[i] = 0;
        
        is_index_block = (bool * ) malloc(sizeof(bool) * BitVectorSize);
        if (is_index_block == NULL) {
            perror("malloc");
            printf("not formatted\n");
            return;
        }
        for (int i = 0; i < BitVectorSize; i++)
            is_index_block[i] = false;

        for (int i = 0; i < MainDir.size(); i++)
                free(MainDir[i].get_fs());
        MainDir.clear();
        OpenFileDescriptors.clear();
        int stack_size=avalible_fd.size();
        for(int i=0;i<stack_size;i++)
            avalible_fd.pop();
        for(int i=BitVectorSize;i>=0;i--)
                avalible_fd.push(i);
        is_formated = true;
        real_zero_index=-1;
    }
    /**
     * CreateFile creates a new file with the given name, and returns the file descriptor of the newly
     * created file
     * 
     * @param fileName the name of the file to be created
     * 
     * @return The index of the file descriptor in the vector.
     */
    int CreateFile(string fileName) {
        if (!is_formated)
            return -1;
        //check if have file with th same name
        bool exist = false;
        for (int i = 0; i < MainDir.size(); i++)
            if (!MainDir[i].getFileName().compare(fileName))
                exist=true;
        if(exist)//have file with the same name - error
            return -1;
        FsFile * to_add = new FsFile(this -> block_size);
        int index_block_for_to_add = get_free_index_block();
        if (index_block_for_to_add < 0) {
            printf("the disk is full!, file not created\n");
            return -1;
        }
        to_add -> set_index_block(index_block_for_to_add);
        FileDescriptor fd = FileDescriptor(fileName, to_add);
        MainDir.push_back(fd);
        is_index_block[index_block_for_to_add]=true;
        return OpenFile(fileName);    
    }  
   /**
    * OpenFile open an existing file
    * 
    * @param filename the name of the file to open
    */
    int OpenFile(string filename) {
        if (!is_formated)
            return -1;
        bool found = false;
        //file not exists in MainDir return -1
        for (int i = 0; i < MainDir.size(); i++)
            if (!filename.compare(MainDir[i].getFileName()))
                found = true;
        if (!found)
            return -1;

        if (OpenFileDescriptors.find(filename) == OpenFileDescriptors.end()) { // file not opened
            if(!avalible_fd.empty()){
                int fd = avalible_fd.top();//get free fd
                avalible_fd.pop();
                OpenFileDescriptors[filename] = fd;//add to the file descriptor table
                return fd;
            }
        }
        //file already opened
        return -1;
    }
    /**
     * It takes a file descriptor as an argument and returns the name of the file that was closed or -1 if filed
     * 
     * @param fd file descriptor
     * 
     * @return The name of the file that was closed.
     */
    string CloseFile(int fd) {
        if (!is_formated)
            return "-1";
        //check if file is open
        string file_to_close;
        bool fd_open = false;
        map < string, int > ::iterator itr;
        for (itr = this -> OpenFileDescriptors.begin(); itr != this -> OpenFileDescriptors.end(); ++itr)
            if (itr -> second == fd) {
                fd_open = true;
                file_to_close = itr -> first;
                break;
            }
        if (!fd_open)
            return "-1";
        avalible_fd.push(OpenFileDescriptors[file_to_close]);
        OpenFileDescriptors.erase(itr);//remove from fd table
        for (int i = 0; i < MainDir.size(); i++)
            if (!MainDir[i].getFileName().compare(file_to_close))
                MainDir[i].set_inUse(false);
        return file_to_close;
    }  
    /**
     * The function gets a file descriptor, a buffer and a length, and writes to the buffer to the content of the file
     * 
     * @param fd file descriptor
     * @param buf the buffer to write from
     * @param len the length of the buffer how many char to read
     * 
     * @return the number of bytes written to the file.
     */
    int WriteToFile(int fd, char * buf, int len) {
        if (!is_formated)
            return -1;
        string file_to_write;
        //check if file is open
        bool fd_open = false;
        map < string, int > ::iterator itr;
        for (itr = this -> OpenFileDescriptors.begin(); itr != this -> OpenFileDescriptors.end(); ++itr)
            if (itr -> second == fd) {
                fd_open = true;
                file_to_write = itr -> first;
                break;
            }
        if (!fd_open)
            return -1;
        //find the file in the main directory
        int j;
        for (int i = 0; i < MainDir.size(); i++)
            if (!MainDir[i].getFileName().compare(file_to_write))
                j = i;
        if (( * (MainDir[j].get_fs())).get_file_size() + len > get_max_file_size()) //over the file max size
            return -1;
        int new_block_nedded = (len - free_space_in_last_block(MainDir[j].get_fs())) / block_size;
        if ((len - free_space_in_last_block(MainDir[j].get_fs())) % block_size != 0)
            new_block_nedded++;
        if (new_block_nedded > get_free_space())//not free space to write 
            return -1;
        int i = 0;

        while (i < len) {
            if (( * (MainDir[j].get_fs())).get_file_size() % block_size != 0) {//have place in the last block 
                int which_block = ( * (MainDir[j].get_fs())).get_file_size() / block_size;//calc the last block allocated
                int offset = (( * (MainDir[j].get_fs())).get_file_size() % block_size);
                int phisical_addr = get_nd_block_index_of_file(MainDir[j].get_fs(), which_block) * block_size + offset;
                replace_char_in_disk(phisical_addr, buf[i]);//writing...
            } else {//need to allocate a new block fo the file
                int phisical_addr_of_new_pointer = ( * (MainDir[j].get_fs())).get_index_block() * block_size + ( * (MainDir[j].get_fs())).get_file_size() / block_size;
                int new_free_block_index = get_free_index_block();
                if(new_free_block_index==0)
                    real_zero_index=phisical_addr_of_new_pointer;
                char index_chared = (int) new_free_block_index;
                replace_char_in_disk(phisical_addr_of_new_pointer, index_chared);//write the index of the new allocted block in index block
                get_nd_block_index_of_file(MainDir[j].get_fs(), 0);
                replace_char_in_disk(new_free_block_index * block_size, buf[i]);//write the char
                ( * (MainDir[j].get_fs())).increase_block_in_use();
            }
            ( * (MainDir[j].get_fs())).increase_file_size();
            i++;
        }
        return len;
    }
    
    /**
     * The function deletes a file from the file system
     * 
     * @param file_to_del the name of the file to delete
     * 
     * @return The file descriptor of the file that was deleted.
     */
    int DelFile(string file_to_del) {
        if (!is_formated)
            return -1;
        //check if file exists
        bool exist = false;
        int j;
        for (int i = 0; i < MainDir.size(); i++){
            if (!MainDir[i].getFileName().compare(file_to_del)){
                j = i;
                exist=true;
            }
        }
        if(!exist)//file not exists...
            return -1;
        if (OpenFileDescriptors.find(file_to_del) == OpenFileDescriptors.end())//file not open
            OpenFile(file_to_del);
        int fd = OpenFileDescriptors[file_to_del];
        int block_to_del_count = (*(MainDir[j].get_fs())).get_file_size() / block_size;//calc how many block to erease
        if ((*(MainDir[j].get_fs())).get_file_size() % block_size != 0)
            block_to_del_count++;
        int deleted = 0;//counter of deleted char
        int len = (*(MainDir[j].get_fs())).get_file_size();//calc how many char to delete
        for (int which_block = 0; which_block < block_to_del_count; which_block++) {//iterate over block to del
            for (int i = 0; i < block_size; i++)//iterate inside the block
                if (block_size * which_block + i > len)
                    break;
                else {
                    int phisical_addr = get_nd_block_index_of_file((MainDir[j].get_fs()), which_block) * block_size + i;//calc the phisical to erase
                    replace_char_in_disk(phisical_addr, '\0');//deleting...
                }
            BitVector[get_nd_block_index_of_file((MainDir[j].get_fs()), which_block)] = 0;//set the bitVector to 0 according to the freed block
        }
        for (int i = 0; i < block_size; i++){//delete the content of the index block
            int index_to_del=(*(MainDir[j].get_fs())).get_index_block() * block_size + i;
            replace_char_in_disk(index_to_del, '\0');
            if(real_zero_index==index_to_del)
                index_to_del=-1;
        }
        BitVector[(*(MainDir[j].get_fs())).get_index_block()] = 0;//set the bitVector to 0 according to the freed block
        avalible_fd.push(OpenFileDescriptors[file_to_del]);
        is_index_block[(*(MainDir[j].get_fs())).get_index_block()] = false;
        //remove the file from MainDir
        map<string, int>::iterator itr;
        itr = OpenFileDescriptors.find(file_to_del);
        OpenFileDescriptors.erase(itr);
        int rem_ind;
        for (int i = 0; i < MainDir.size(); i++)
            if (!MainDir[i].getFileName().compare(file_to_del)){
                rem_ind = i;
                break;
            }
        free(MainDir[rem_ind].get_fs());
        vector < FileDescriptor > ::iterator it;
        it = MainDir.begin() + rem_ind;
        MainDir.erase(it);
        
        return fd;

    }
    /**
     * Reads from a file, given a file descriptor, a buffer and a length to read
     * 
     * @param fd file descriptor
     * @param buf the buffer to read into
     * @param len the length of the buffer
     * 
     * @return the number of bytes read from the file.
     */
    int ReadFromFile(int fd, char * buf, int len) {
        if (!is_formated)
            return -1;
        //check if file is open
        string file_to_read;
        bool fd_open = false;
        map < string, int > ::iterator itr;
        for (itr = this -> OpenFileDescriptors.begin(); itr != this -> OpenFileDescriptors.end(); ++itr)
            if (itr -> second == fd) {
                fd_open = true;
                file_to_read = itr -> first;
                break;
            }
        if (!fd_open)//not open - error
            return -1;
        int j;
        for (int i = 0; i < MainDir.size(); i++)
            if (!MainDir[i].getFileName().compare(file_to_read))
                j = i;
        //FsFile * fs_to_read = get_fs_by_name(file_to_read);
        if (len > (*(MainDir[j].get_fs())).get_file_size())//try to read more than the file size
            return -1;
        int block_to_read_count = len / block_size;
        if (len % block_size != 0)
            block_to_read_count++;
        int buf_ind = 0;
        for (int which_block = 0; which_block < block_to_read_count; which_block++)
            for (int i = 0; i < block_size; i++)
                if (block_size * which_block + i >= len)
                    break;
                else {
                    int phisical_addr = get_nd_block_index_of_file(MainDir[j].get_fs(), which_block) * block_size + i;
                    buf[buf_ind++] = get_char_from_disk(phisical_addr);
                }
        buf[len]='\0';
        return len;
    }
    private:
    /**
     * return the index of the #-nd block of the file
     * 
     * @param to_write the file to write to
     * @param which_block the index of the block in the file
     * 
     * @return The index of the block in the disk.
     */
    int get_nd_block_index_of_file(FsFile * to_write, int which_block) {
        char c[DISK_SIZE];
        int index_to_read = to_write -> get_index_block() * block_size + which_block;
        fseek(sim_disk_fd, 0, SEEK_SET);
        fread(c, sizeof(c), 1, sim_disk_fd);
        return (int) c[index_to_read];
    }
    /**
     * "Read single char from disk at index @param addr
     * 
     * @param addr the address of the character you want to read from the disk
     * 
     * @return The character at the address specified by the parameter.
     */
    char get_char_from_disk(int addr) {
        char c[DISK_SIZE];
        fseek(sim_disk_fd, 0, 0);
        fread(c, sizeof(c), 1, sim_disk_fd);
        return c[addr];
    }
    /**
     * calc free space in the last block of the file
     * 
     * @param to_check the file to check
     * 
     * @return The amount of free space in the last block of the file.
     */
    int free_space_in_last_block(FsFile * to_check) {
        return block_size - to_check -> get_file_size() % block_size;
    }
    /**
     * It returns the number of free blocks in the disk.
     * 
     * @return The number of free blocks in the disk.
     */
    int get_free_space() {
        int res = 0;
        for (int i = 0; i < BitVectorSize; i++)
            if (BitVector[i] == 0)
                res++;
        return res;
    }
    /**
     * 
     * @return The maximum file size in the current system format.
     */
    int get_max_file_size() {
        return block_size * block_size;
    }
    /**
     * It returns the index of the first free block in the BitVector.
     * 
     * @return The index of the first free block in the bit vector.
     */
    int get_free_index_block() {
        for (int i = 0; i < BitVectorSize; i++)
            if (BitVector[i] == 0) {
                BitVector[i] = 1;
                return i;
            }
        return -1;
    }
    /**
     *
     * write @param c at @param index in the disk
     * 
     * @param index the index of the character to be replaced
     * @param c the character to replace
     */
    void replace_char_in_disk(int index, char c) {
        char disk_buff[DISK_SIZE];
        fseek(sim_disk_fd, 0, SEEK_SET);
        fread(disk_buff, sizeof(disk_buff), 1, sim_disk_fd);
        disk_buff[index] = c;
        fclose(sim_disk_fd);
        std::ofstream ofs;
        ofs.open(DISK_SIM_FILE, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
        sim_disk_fd = fopen(DISK_SIM_FILE, "r+");
        fseek(sim_disk_fd, 0, SEEK_SET);
        fwrite(disk_buff, sizeof(disk_buff), 1, sim_disk_fd);
    }
};

int main() {
    int blockSize;
    int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;

    fsDisk * fs = new fsDisk();
    int cmd_;
    while (1) {
        cin >> cmd_;
        switch (cmd_) {
        case 0: // exit
            delete fs;
            exit(0);
            break;

        case 1: // list-file
            fs -> listAll();
            break;

        case 2: // format
            cin >> blockSize;
            fs -> fsFormat(blockSize);
            break;

        case 3: // creat-file
            cin >> fileName;
            _fd = fs -> CreateFile(fileName);
            cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

        case 4: // open-file
            cin >> fileName;
            _fd = fs -> OpenFile(fileName);
            cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

        case 5: // close-file
            cin >> _fd;
            fileName = fs -> CloseFile(_fd);
            cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;

        case 6: // write-file
            cin >> _fd;
            cin >> str_to_write;
            fs -> WriteToFile(_fd, str_to_write, strlen(str_to_write));
            break;

        case 7: // read-file
            cin >> _fd;
            cin >> size_to_read;
            fs -> ReadFromFile(_fd, str_to_read, size_to_read);
            cout << "ReadFromFile: " << str_to_read << endl;
            break;

        case 8: // delete file 
            cin >> fileName;
            _fd = fs -> DelFile(fileName);
            cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
            break;
        default:
            break;
        }
    }
    return 0;
}