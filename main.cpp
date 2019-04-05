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
#define DocVec std::vector<float>
int VOCAB_SZ;
int DOC_SZ;


int load_vocab(std::string const& name, lexiTree& trie){
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


void load_raw_TF(std::string const& name, std::vector<int>& maxFreq, std::vector<DocVec>& tfmat, std::vector<float>& IDF){
    std::fstream reader(name, std::fstream::in);
    std::string sentence;

    int coll_sz = tfmat.size();
    int vid_1 = -1, vid_2 = -1, N = 0;
    while(std::getline(reader, sentence)){
        if(N==0){
            if(!(std::stringstream(sentence) >> vid_1 >> vid_2 >> N)) exit(1);
            assert(vid_1 >= 0 && vid_1 < VOCAB_SZ);
            IDF[vid_1] = std::log(coll_sz/(float)N);
            // cout << "expec N = " << N << endl;
        }else{

            // cout << "N = " << N << endl;

            int file_id = -1, tf = 0;

            if(!(std::stringstream(sentence) >> file_id >> tf)) exit(1);

            // cout << file_id << " " << tf << endl;
            assert(file_id >= 0 && file_id < DOC_SZ && tf >= 0);

            // TODO: bi-gram, doc-len
            if(vid_2 == -1){            
                maxFreq[file_id] = std::max(maxFreq[file_id], tf);
                tfmat[file_id][vid_1] = tf;
            }

            N--;
        }
        
    }
}

#define alpha 0.5
int normalize_matrix(std::vector<int>const& maxFreq, std::vector<float>const& IDF, std::vector<DocVec>& tfmat){
    for(int i = 0; i < DOC_SZ; i++){
        float norm = 0.;
        if(maxFreq[i]<=0)
            cout << "WARNING: file with id " << i << " has 0 term frequency." << endl;
        for(int j = 0; j < VOCAB_SZ; j++){
            float TF = 1-alpha + alpha*tfmat[i][j]/(float)maxFreq[i];
            // float TF = tfmat[i][j]/(float)maxFreq[i];
            tfmat[i][j] = TF*IDF[j];
            norm += tfmat[i][j]*tfmat[i][j];
        }

        norm = std::sqrt(norm);
        for(int j = 0; norm>0 && j < VOCAB_SZ; j++){
            tfmat[i][j] = tfmat[i][j]/norm;
        }
    }
}

float dot(DocVec const& a, DocVec const& b){
    return std::inner_product(a.begin(), a.end(), b.begin(), 0.);
}

class Query{
public:
    std::string qid;
    DocVec qvec;
    // int maxFreq;

    Query(tinyxml2::XMLElement* topicElement, lexiTree& voc, std::vector<float>const& IDF){
        std::string number = topicElement->FirstChildElement("number")->GetText();
        std::string title = topicElement->FirstChildElement("title")->GetText();
        std::string question = topicElement->FirstChildElement("question")->GetText();
        std::string narrative = topicElement->FirstChildElement("narrative")->GetText();
        std::string concepts = topicElement->FirstChildElement("concepts")->GetText();

        this->maxFreq = 0;
        qid = std::string(number.end()-3, number.end());
        qvec = DocVec(VOCAB_SZ, 0.);
        _process(title, voc);
        _process(question, voc);
        _process(narrative, voc);
        _process(concepts, voc);

        float norm = 0.;
        for(int j = 0; j < VOCAB_SZ; j++){
            float TF = 1-alpha + alpha*qvec[j]/(float)maxFreq;
            qvec[j] = TF*IDF[j];
            norm += qvec[j]*qvec[j];
        }

        norm = std::sqrt(norm);
        for(int j = 0; norm>0 && j < VOCAB_SZ; j++){
            qvec[j] = qvec[j]/norm;
        }
    }
private:    
    int maxFreq;
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
            if(index>=0 && index < VOCAB_SZ){
                qvec[index] += 1;
                maxFreq = std::max(maxFreq, (int)qvec[index]);
            }
        }
        while ( str_i < end );
    }
};

int main(int argc, char** argv){
    std::string vocab_name = "vocab.all";
    std::string doclist_name = "file-list";
    std::string freq_name = "inverted-file";
    std::string query_name = "query-train.xml";
    // std::string query_name = "query-test.xml";
    std::string output_name = "ans.csv";
    lexiTree vocab;
    // lexiTree docname;
    std::vector<std::string> docname;
    

    VOCAB_SZ = load_vocab(vocab_name, vocab);
    cout << "[info] Vocab size: " << VOCAB_SZ << endl;
    
    DOC_SZ = load_filenames(doclist_name, docname);
    cout << "[info] Document list size: " << DOC_SZ << endl;

    // // load raw term frequency
    std::vector<DocVec> TF_matrix;
    std::vector<float> IDF(VOCAB_SZ, 0.);
    std::vector<int> maxFreq(DOC_SZ, 0);
    for(int i = 0; i < DOC_SZ; i++){
     TF_matrix.push_back(DocVec(VOCAB_SZ, 0.));  
    }
    load_raw_TF(freq_name, maxFreq, TF_matrix, IDF);

    // matrix normalization  
    normalize_matrix(maxFreq, IDF, TF_matrix);

    // cout << dot(TF_matrix[0], TF_matrix[1]) << endl;

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
        Query query(topicElement, vocab, IDF);
        DocVec qvec = query.qvec;

        maxScores.clear();
        for(int i = 0; i < DOC_SZ; i++){
            float score = dot(TF_matrix[i], qvec);
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
