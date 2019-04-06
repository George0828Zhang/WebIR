#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cassert>
#include <numeric> //inner_product
// #include <expat.h>
#include "tinyxml2/tinyxml2.h"
#include "utfcpp/source/utf8.h"
#include "Tree.hpp"
#include "Array.hpp"

#define MAXCAND 100
#define cout std::cout
#define endl std::endl
int VOCAB_SZ;
int TERM_SZ;
int DOC_SZ;


class Document{
public:
    std::vector<float> vec;
    int dim;
    int id;
    int maxFreq;
    int length;
    Document(int i, int d): id(i), dim(d), maxFreq(0), length(0), vec(d, 0.){ };
    void update(int index, int freq){
        vec[index] += freq;
        maxFreq = std::max(maxFreq, (int)vec[index]);
        length += freq;
    }
    void normalize0(std::vector<float>const& IDF){
        float alpha = 0.5;
        float norm = 0.;
        if(maxFreq<=0)
            cout << "WARNING: file with id " << id << " has 0 term frequency." << endl;
        for(int j = 0; j < dim; j++){
            float TF = 1-alpha + alpha*vec[j]/(float)maxFreq;
            vec[j] = TF*IDF[j];
            norm += vec[j]*vec[j];
        }

        norm = std::sqrt(norm);
        for(int j = 0; norm>0 && j < dim; j++){
            vec[j] /= norm;
        }
    }
    void normalize(int avgdl, std::vector<float>const& IDF){
        float s = 0.2;
        float norm = 0.;
        if(maxFreq<=0)
            cout << "WARNING: file with id " << id << " has 0 term frequency." << endl;
        float TF, dlen_norm;
        for(int j = 0; j < dim; j++){
            if(vec[j]>0){
                TF = 1 + std::log(1 + std::log(vec[j]));
                dlen_norm = 1-s + s*(length/avgdl);
                vec[j] = TF/dlen_norm * IDF[j];
                norm += vec[j]*vec[j];
            }
        }

        norm = std::sqrt(norm);
        for(int j = 0; norm>0 && j < dim; j++){
            vec[j] /= norm;
        }
    }
};


int load_vocab_t(std::string const& name, lexiTree& trie){
    std::fstream reader(name, std::fstream::in);
    std::string encoding;
    reader >> encoding;
    cout << "[info] Vocab encoding: " << encoding << endl;
    int n_word = 0;
    std::string word;
    while(std::getline(reader, word)){
        trie.insert(word, n_word);
        n_word++;
    }
    return n_word;
}

int load_vocab(std::string const& name, std::vector<std::string>& dict){
    std::fstream reader(name, std::fstream::in);
    std::string encoding;
    reader >> encoding;
    cout << "[info] Vocab encoding: " << encoding << endl;
    int n_word = 0;
    std::string word;
    while(std::getline(reader, word)){
        dict.push_back(word);
        n_word++;
    }
    return n_word;
}

void lower(std::string& in){
    for(auto& c : in){
        c = std::tolower(c);
    }
}

int load_filenames(std::string const& name, std::vector<std::string>& dict){
    std::fstream reader(name, std::fstream::in);
    int n_word = 0;
    std::string word;
    while(std::getline(reader, word)){
        size_t start = word.rfind('/')+1;
        std::string tmp(word.begin()+start, word.end());
        lower(tmp);
        dict.push_back(tmp);
        n_word++;
    }
    return n_word;
}


void load_raw_TF(
    std::string const& name, 
    std::vector<Document>& tfdocs,
    std::vector<float>& IDF)
{
    assert(IDF.size()==TERM_SZ);
    assert(tfdocs.size()==DOC_SZ);


    std::fstream reader(name, std::fstream::in);
    std::string sentence;

    int vid_1 = -1, vid_2 = -1, N = 0;
    while(std::getline(reader, sentence)){
        if(N==0){
            if(!(std::stringstream(sentence) >> vid_1 >> vid_2 >> N)) exit(1);
            assert(vid_1 >= 0 && vid_1 < VOCAB_SZ);
            IDF[vid_1] = std::log((DOC_SZ+1)/(float)N);
        }else{

            int file_id = -1, tf = 0;

            if(!(std::stringstream(sentence) >> file_id >> tf)) exit(1);

            assert(file_id >= 0 && file_id < DOC_SZ && tf >= 0);

            // Unigram
            if(vid_2 == -1){
                tfdocs[file_id].update(vid_1, tf);
            }

            // TODO: bi-gram

            N--;
        }
        
    }
}

float dot(std::vector<float> const& a, std::vector<float> const& b){
    return std::inner_product(a.begin(), a.end(), b.begin(), 0.);
}

class Query : public Document {
public:
    std::string qid;

    Query(int Dim, tinyxml2::XMLElement* topicElement, 
        lexiTree& voc, 
        std::vector<float>const& IDF)
    : Document(-1, Dim)
    // :maxFreq(0), length(0), dim(Dim), vec(Dim, 0.)
    {
        std::string number = topicElement->FirstChildElement("number")->GetText();
        std::string title = topicElement->FirstChildElement("title")->GetText();
        std::string question = topicElement->FirstChildElement("question")->GetText();
        std::string narrative = topicElement->FirstChildElement("narrative")->GetText();
        std::string concepts = topicElement->FirstChildElement("concepts")->GetText();

        qid = std::string(number.end()-3, number.end());
        _process(title, voc);
        _process(question, voc);
        _process(narrative, voc);
        _process(concepts, voc);

        // normalize(IDF);
    }
private:
    void _process(std::string const& text, lexiTree& voc){
        // reference: https://stackoverflow.com/questions/2852895/c-iterate-or-split-utf-8-string-into-array-of-symbols
        char* str = (char*)text.c_str();    // utf-8 string
        char* str_i = str;                  // string iterator
        char* end = str+strlen(str)+1;      // end iterator

        char symbol[] = {0,0,0,0,0};

        do
        {
            uint32_t code = utf8::next(str_i, end); // get 32 bit code of a utf-8 symbol
            if (code == 0)
                continue;
            char* end = utf8::append(code, symbol); // initialize array `symbol`
            
            std::string strsym(symbol, end);
            // cout << strsym << " ";
            int index = voc.wordIndex(strsym);
            if(index>=0 && index < TERM_SZ){
                update(index, 1);
            }
        }
        while ( str_i < end );
    }
};

float get_avg_doclen(std::vector<Document> const& docs){
    float a = 0.;
    for(auto& d : docs){
        a += d.length;
    }
    return a / docs.size();
}

int main(int argc, char** argv){
    std::string vocab_name = "vocab.all";
    std::string doclist_name = "file-list";
    std::string freq_name = "inverted-file";
    // std::string query_name = "query-train.xml";
    std::string query_name = "query-test.xml";
    std::string output_name = "ans.csv";
    lexiTree vocab;
    // lexiTree docname;
    std::vector<std::string> docname;
    

    VOCAB_SZ = load_vocab_t(vocab_name, vocab);
    cout << "[info] Vocab size: " << VOCAB_SZ << endl;
    TERM_SZ = VOCAB_SZ;
    
    DOC_SZ = load_filenames(doclist_name, docname);
    cout << "[info] Document list size: " << DOC_SZ << endl;

    // // load raw term frequency
    std::vector<Document> TF_docs;
    std::vector<float> IDF(TERM_SZ, 0.);
    for(int i = 0; i < DOC_SZ; i++){
        TF_docs.push_back(Document(i, TERM_SZ));
    }
    load_raw_TF(freq_name, TF_docs, IDF);

    float avgdl = get_avg_doclen(TF_docs);
    // matrix normalization
    for(auto& d : TF_docs){
        d.normalize(avgdl, IDF);
    }

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError eResult = doc.LoadFile(&query_name[0]);
    tinyxml2::XMLElement* topicElement = doc.FirstChildElement("xml")->FirstChildElement("topic");
    if (topicElement == nullptr) cout << "ERROR: XML element parsing error." << endl;
    std::vector<Query> QList;
    Kmax<float, int> maxScores(MAXCAND);
    std::fstream writer(output_name, std::fstream::out);
    writer << "query_id,retrieved_docs";
    while (topicElement != nullptr)
    {
        Query query(TERM_SZ, topicElement, vocab, IDF);

        maxScores.clear();
        for(int i = 0; i < DOC_SZ; i++){
            float score = dot(TF_docs[i].vec, query.vec);
            maxScores.insert(score, i);
        }
        std::vector<int> top_candidates;
        maxScores.sort();
        maxScores.print();
        maxScores.extract(top_candidates);

        writer << endl << query.qid << ",";
        for(auto id : top_candidates){
            writer << docname[id] << " ";
        }

        topicElement = topicElement->NextSiblingElement("topic");
    }
}
