#include "SuffixTree.h"
#include <limits>
#include "stringify.h"

using namespace std;

class SearchTrans {

public:

  SearchTrans(bool uninit=false) : aggregation(1),
                                   symbol_count(0),
                                   symbol_cache(0),
                                   transcoded_bits(8),
                                   st(uninit) {
  }

  // this is a placeholder, I may use it to reduce the bit count later.
  uint8_t transcode_char(uint8_t c) {
   /* c = toupper(c);
    if((c >= 'A') && (c <= 'Z')) {
      c = c - 'A' + 1;
    } else {
      c = 0;
    }
    return c;
    */
    return c;
  }

  uint16_t join(uint16_t a,uint8_t b) {
    a = a << transcoded_bits;
    a = a + b;
    return a;
  }

  vector<uint16_t> transcode(vector<uint8_t> s,uint8_t offset=0) {

    vector<uint16_t> s_trans;

    uint16_t symbol_sum=0;
    uint8_t  symbol_count=0;
    for(size_t n=offset;n<s.size();n++) {
      if(symbol_count==aggregation) {
        s_trans.push_back(join(symbol_sum,transcode_char(s[n])));
        symbol_count=0;
        symbol_sum=0;
      } else {
        symbol_sum = join(symbol_sum,transcode_char(s[n]));
        symbol_count++;
      }
    }
    return s_trans;
  }

  bool validate_hit(size_t hit,const vector<uint8_t> &s,int offset) {

    size_t original_position = hit*(aggregation+1);

    //cout << "Original Text: ";
    //for(int n=0;n<20;n++) {
    //  cout << original_text[original_position-offset+n];
    //}
    //cout << endl;

    //cout << "Search string: ";
    //for(size_t n=0;n<s.size();n++) {
    //  cout << s[n]; 
    //}
    //cout << endl;

    //validate missing start positions
    for(int n=0;n<offset;n++) {
      if(s[n] != original_text[original_position-offset+n]) return false;
    }

    //validate missing end positions
    size_t extra=(s.size()%(aggregation+1))+offset;

    for(size_t n=s.size()-extra;n<s.size();n++) {
      if(s[n] != original_text[original_position+n-offset]) return false;
    }

    return true;
  }

  void validation_filter(vector<size_t> &hits,const vector<uint8_t> &s,int offset) {
    if((offset == 0) && ((s.size() % (aggregation+1)) == 0)) return;

    for(size_t n=0;n<hits.size();n++) {
      bool valid = validate_hit(hits[n],s,offset); 
      if(!valid) hits[n]=numeric_limits<size_t>::max();
    }
  }

  void process_positions() {
    st.process_positions();
  }

  vector<size_t> all_occurs(vector<uint8_t> ss,size_t max_hits=-1) {

    if(ss.size() < (aggregation+1)) return vector<size_t>(); // we can't search for things this short.

    vector<size_t> all_hits;
    for(size_t offset=0;offset<=aggregation;offset++) {

      vector<uint16_t> trans = transcode(ss,offset);
      //for(size_t n=0;n<trans.size();n++) {
      //  uint16_t s = trans[n];
      //}

      vector<size_t> hits = st.all_occurs(transcode(ss,offset),max_hits);
      validation_filter(hits,ss,offset);
      for(size_t n=0;n<hits.size();n++) if(hits[n]!=numeric_limits<size_t>::max()) all_hits.push_back((hits[n]*(aggregation+1)-offset));
    }

    return all_hits;
  }

  bool exists(vector<uint8_t> t) {

    if(t.size() < (aggregation+1)) return false; // we can't search for things this short.
    return false; // not sure I'm going to support exists from here...
  }

  void insert(uint8_t current_symbol) {
    
    original_text.push_back(current_symbol);

    if(symbol_count == aggregation) {
      st.insert(join(symbol_cache,transcode_char(current_symbol)));
      symbol_count = 0;
      symbol_cache = 0;
    } else {
      symbol_cache = join(symbol_cache,transcode_char(current_symbol));
      symbol_count++;
    }
  }

  void finalise() {
    if(symbol_count == aggregation) {
      st.insert(join(symbol_cache,0));
      symbol_count = 0;
      symbol_cache = 0;
      original_text.push_back(0);
    }
 
    uint16_t t = final_symbol;
    uint8_t t1 = t >> transcoded_bits;
    uint8_t t2 = 0x00FF & t;
 
    original_text.push_back(t1);
    original_text.push_back(t2);
    st.finalise();
  }
  
  void compact() {
    st.compact();
  }

  void dump_stats() {
    st.dump_stats();
  }

  size_t size() {
    return original_text.size();
  }

  string get_substr(size_t start,size_t end) {
    string sub;
    for(size_t n=start;n<=end;n++) sub += original_text[n];
    return sub;
  }

  suffixnodestore_type &get_store() {
    return st.store;
  }

  void save_members(string filename) {
    // write my members
    ofstream membersfile(filename.c_str(),ios_base::app); // open for append
    membersfile << "searchtrans_aggregation=" << (int) aggregation << endl;
    membersfile << "searchtrans_symbol_count=" << (int) symbol_count << endl;
    membersfile << "searchtrans_symbol_cache=" << (int) symbol_cache << endl;
    membersfile << "searchtrans_transcoded_bits=" << (int) transcoded_bits << endl;
    membersfile.close();

    st.save_members(filename);
  }

  void load_members(string filename) {
    ifstream membersfile(filename.c_str());
  
    for(;!membersfile.eof();) {
      string line;
      getline(membersfile,line);

      stringstream cline(line);

      string member;
      string value;
      getline(cline,member,'=');
      getline(cline,value);

      if(member == "searchtrans_aggregation"    ) aggregation     = convertTo<uint16_t>(value);
      if(member == "searchtrans_symbol_count"   ) symbol_count    = convertTo<uint16_t>(value);
      if(member == "searchtrans_symbol_cache"   ) symbol_cache    = convertTo<uint16_t>(value);
      if(member == "searchtrans_trnascoded_bits") transcoded_bits = convertTo<uint16_t>(value);
    }
    membersfile.close();

    st.load_members(filename);
  }

  void save_original_text(string filename) {
    ofstream textfile(filename.c_str());
 
    for(size_t n=0;n<original_text.size();n++) {
      textfile << original_text[n];
    }
    textfile.close();
  }

  uint8_t  aggregation; 
  uint8_t  symbol_count;
  uint16_t symbol_cache;
  uint8_t  transcoded_bits;

  searchtrans_store_type original_text;
  SuffixTree st;
};
