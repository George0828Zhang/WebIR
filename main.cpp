#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cassert>
#include "tinyxml2/tinyxml2.h"
#include "utfcpp/source/utf8.h"
#include "Utils.hpp"

#define Okapi_k1 1.0
#define Okapi_b 0.35
#define Okapi_k3 5.0

#define R_a 0.7
#define R_b 0.15

#define MAXCAND 100
#define cout std::cout
#define endl std::endl
int VOCAB_SZ;
int DOC_SZ;


class Document{
public:
    std::vector<int> tid;
    std::vector<float> freq;
    int id;
    int maxFreq;
    int length;
    Document(int docid): id(docid), maxFreq(0), length(0) { };
    
    void normalize(int avgdl, std::vector<float>const& IDF){
        float norm = 0.;
        if(maxFreq<=0)
            cout << "WARNING: file with id " << id << " has 0 term frequency." << endl;
        float TF, dlen_norm;
        for(int j = 0; j < tid.size(); j++){
            TF = (Okapi_k1+1)*freq[j];
            dlen_norm = Okapi_k1*(1 - Okapi_b + Okapi_b * (length/avgdl)) + freq[j];
            freq[j] = TF/dlen_norm * IDF[tid[j]];
            norm += freq[j]*freq[j];
        }

        norm = std::sqrt(norm);
        for(int j = 0; norm>0 && j < tid.size(); j++){
            freq[j] /= norm;
        }
    }
};

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
    std::vector<std::string> const& vocab,
    std::vector<Document>& tfdocs,
    std::vector<float>& IDF,
    lexiTree& termDic)
{
    assert(IDF.size()==0);
    assert(tfdocs.size()==0);
    assert(termDic.size==0);

    for(int i = 0; i < DOC_SZ; i++){
        tfdocs.push_back(Document(i));       
    }

    std::fstream reader(name, std::fstream::in);
    std::string sentence;
    int term_id = -1;
    int vid_1, vid_2, N = 0;
    int file_id, tf;


    while(std::getline(reader, sentence)){
        if(N==0){
            vid_1 = -1;
            vid_2 = -1;

            if(!(std::stringstream(sentence) >> vid_1 >> vid_2 >> N)) exit(1);
            assert(vid_1 >= 0 && vid_1 < VOCAB_SZ);
            // if(vid_2==-1) continue;
            std::string term;
            if(vid_2==-1)
                term = vocab[vid_1];
            else
                term = vocab[vid_1] + " " + vocab[vid_2];

            term_id = IDF.size();
            termDic.insert(term, term_id);
            // pivoted normalization
            // IDF.push_back(std::log((DOC_SZ+1)/(float)N));
            // Okapi
            IDF.push_back(std::log((DOC_SZ - N + 0.5)/(N + 0.5)));
        }else{

            file_id = -1;
            tf = 0;

            if(!(std::stringstream(sentence) >> file_id >> tf)) exit(1);

            assert(file_id >= 0 && file_id < DOC_SZ && tf >= 0);

            if(vid_2 == -1){
                // Unigram
                tfdocs[file_id].tid.push_back(term_id);
                tfdocs[file_id].freq.push_back(tf);
                tfdocs[file_id].maxFreq = std::max(tfdocs[file_id].maxFreq, tf);
                tfdocs[file_id].length += tf;
            }else{
                // Bigram
                tfdocs[file_id].tid.push_back(term_id);
                tfdocs[file_id].freq.push_back(tf);
                tfdocs[file_id].maxFreq = std::max(tfdocs[file_id].maxFreq, tf);
            }

            N--;
        }
        
    }
}

class Query {
public:
    std::string qid;
    std::vector<float> vec;
    int dim;
    int id;
    int maxFreq;
    int length;

    Query(tinyxml2::XMLElement* topicElement, 
        lexiTree const& voc)
    :maxFreq(0), length(0), dim(voc.size), vec(voc.size, 0.)
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

        normalize();
    }
    float match(Document& doc){
        float out = 0.;
        for(int i = 0; i < doc.tid.size(); i++){
            out += this->vec[doc.tid[i]] * doc.freq[i];
        }
        return out;
    }
private:
    void update(std::string const& str, lexiTree const& voc){
        int index = voc.wordIndex(str);

        if(index>=0 && index < vec.size()){
            vec[index]++;
            maxFreq = std::max(maxFreq, (int)vec[index]);
        }
    }
    void normalize(){
        for(int j = 0; j < dim; j++){
            float& qtf = vec[j];
            qtf = ((Okapi_k3+1)*qtf / (Okapi_k3+qtf));
        }
    }
    void _process(std::string const& text, lexiTree const& voc){
        // reference: https://stackoverflow.com/questions/2852895/c-iterate-or-split-utf-8-string-into-array-of-symbols
        char* str = (char*)text.c_str();    // utf-8 string
        char* str_i = str;                  // string iterator
        char* end = str+strlen(str)+1;      // end iterator

        char symbol[20] = {};
        std::string lastword = "";
        do
        {
            uint32_t code = utf8::next(str_i, end); // get 32 bit code of a utf-8 symbol
            if (code == 0)
                continue;
            char* end = utf8::append(code, symbol); // initialize array `symbol`
            
            std::string word(symbol, end);
            // cout << lastword + word << " ";
            update(word, voc);
            if(lastword.size()>0)
                update(lastword+" "+word, voc);
            lastword = word;
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

void load_queries(
    std::string const& name,
    lexiTree const& vocab,
    std::vector<Query>& qlist
    )
{
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError eResult = doc.LoadFile(name.c_str());
    tinyxml2::XMLElement* topicElement = doc.FirstChildElement("xml")->FirstChildElement("topic");
    if (topicElement == nullptr) cout << "ERROR: XML element parsing error." << endl;
    
    while (topicElement != nullptr){
        qlist.push_back(Query(topicElement, vocab));
        topicElement = topicElement->NextSiblingElement("topic");
    }
}

void feedback(
    Query& query,
    std::vector<Document> const& tfdocs,
    std::vector<int> const& rel
    )
{
    std::vector<bool> relmap(DOC_SZ, false);
    for(auto i: rel)
        relmap[i] = true;
    std::vector<float>& vec = query.vec;
    std::vector<float> pos(vec.size(), 0.);
    std::vector<float> neg(vec.size(), 0.);

    for(int i = 0; i < DOC_SZ; i++){
        auto const& doc = tfdocs[i];
        for(int j = 0; j < doc.tid.size(); j++){
            if(relmap[i]){
                pos[doc.tid[j]] += doc.freq[j];
            }else{
                neg[doc.tid[j]] += doc.freq[j];
            }
        }
    }

    for(int j = 0; j < vec.size(); j++){
        vec[j] = R_a*vec[j] + R_b*pos[j]/rel.size() - (1. - R_a - R_b)*neg[j]/(DOC_SZ - rel.size());
    }
}

int main(int argc, char** argv){
    std::string vocab_name = "vocab.all";
    std::string doclist_name = "file-list";
    std::string freq_name = "inverted-file";
    std::string query_name = "query-train.xml";
    // std::string query_name = "query-test.xml";
    std::string output_name = "ans.csv";
    // lexiTree vocab;
    // lexiTree docname;
    std::vector<std::string> vocab;
    std::vector<std::string> docname;
    lexiTree bigram;
    

    VOCAB_SZ = load_vocab(vocab_name, vocab);
    cout << "[info] Vocab size: " << VOCAB_SZ << endl;
    
    DOC_SZ = load_filenames(doclist_name, docname);
    cout << "[info] Document list size: " << DOC_SZ << endl;

    // // load raw term frequency
    std::vector<Document> TF_docs;
    std::vector<float> IDF;

    load_raw_TF(freq_name, vocab, TF_docs, IDF, bigram);
    int TERM_SZ = IDF.size();
    cout << "[info] Term size: " << TERM_SZ << endl;
    assert(TERM_SZ==bigram.size);
    float avgdl = get_avg_doclen(TF_docs);
    // matrix normalization
    for(auto& d : TF_docs){
        d.normalize(avgdl, IDF);
    }

    std::vector<Query> QList;
    load_queries(query_name, bigram, QList);
    Kmax<float, int> maxScores(MAXCAND);


    std::fstream writer(output_name, std::fstream::out);
    writer << "query_id,retrieved_docs";

    for(int q = 0; q < QList.size(); q++){
        auto& query = QList[q];
        std::vector<int> top_candidates;

        maxScores.clear();
        for(int i = 0; i < DOC_SZ; i++){
            float score = query.match(TF_docs[i]);
            maxScores.insert(score, i);
        }        
        maxScores.extract(top_candidates);

        feedback(query, TF_docs, top_candidates);

        maxScores.clear();
        for(int i = 0; i < DOC_SZ; i++){
            float score = query.match(TF_docs[i]);
            maxScores.insert(score, i);
        }
        maxScores.sort();
        maxScores.extract(top_candidates);

        writer << endl << query.qid << ",";
        for(auto id : top_candidates){
            writer << docname[id] << " ";
        }

        std::string bar(30, '.');
        std::fill(bar.begin(), bar.begin() + (int)std::floor(30*(q+1) / QList.size()),'=');
        std::cerr << "(" << q+1 << "/" << QList.size() << ") queries done. Progress [" << bar << "]\r";
    }
}
