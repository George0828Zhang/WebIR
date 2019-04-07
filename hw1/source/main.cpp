#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>// std::sort
#include <unordered_map>
#include "tinyxml2/tinyxml2.h"
#include "utfcpp/source/utf8/checked.h"
#include "utfcpp/source/utf8/core.h"
#include "Utils.hpp"

#define Okapi_k1 1.2
#define Okapi_b 0.75
#define Okapi_k3 1000.
#define IDF_epsilon 1e-4

#define R_a 0.99
#define R_b 0.009
#define R_c 0.001
#define R_rounds 0

#define MAXCAND 100
#define cout std::cout
#define endl std::endl
int VOCAB_SZ;
int DOC_SZ;

using UMap = std::unordered_map<std::string, int>;

class Document{
public:
    std::vector<int> tid;
    std::vector<float> freq;
    int id;
    int length;
    Document(int docid): id(docid), length(0) { };
    
    void normalize(int avgdl, std::vector<float>const& IDF){
        float norm = 0.;
        float TF, dlen_norm;

        for(int j = 0; j < tid.size(); j++){
            TF = (Okapi_k1+1.)*freq[j];
            dlen_norm = Okapi_k1*(1. - Okapi_b + Okapi_b * (length/(float)avgdl)) + freq[j];
            freq[j] = TF/dlen_norm * IDF[tid[j]];
            assert(freq[j] > 0);
            norm += freq[j]*freq[j];
        }

        // norm = std::sqrt(norm);
        // for(int j = 0; norm>0 && j < tid.size(); j++){
        //     freq[j] /= norm;
        // }
    }
};
class Query {
public:
    std::string qid;
    std::vector<float> vec;
    int dim;
    int id;
    int length;

    Query(tinyxml2::XMLElement* topicElement, 
        // lexiTree const& voc)
        UMap const& voc)
    :length(0), dim(voc.size()), vec(voc.size(), 0.)
    {
        std::string number = topicElement->FirstChildElement("number")->GetText();
        std::string title = topicElement->FirstChildElement("title")->GetText();
        std::string question = topicElement->FirstChildElement("question")->GetText();
        std::string narrative = topicElement->FirstChildElement("narrative")->GetText();
        std::string concepts = topicElement->FirstChildElement("concepts")->GetText();

        qid = std::string(number.end()-3, number.end());
        _process(title, voc);
        // _process(question, voc);
        // _process(narrative, voc);
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
    void normalize(){
        for(int j = 0; j < dim; j++){
            vec[j] = ((Okapi_k3+1.)*vec[j] / (float)(Okapi_k3+vec[j]));
        }
    }
    int ufind(UMap const& m, std::string const& w){
        auto i = m.find(w);
        return (i==m.end()) ? -1 : i->second;
    }
    void _process(std::string const& text, UMap const& voc){//, lexiTree const& voc){
        // reference: https://stackoverflow.com/questions/2852895/c-iterate-or-split-utf-8-string-into-array-of-symbols
        char* str = (char*)text.c_str();    // utf-8 string
        char* str_i = str;                  // string iterator
        char* strend = str+strlen(str)+1;      // end iterator

        char symbol[20] = {};
        std::string lastword = "<sos>";
        do
        {
            uint32_t code = utf8::next(str_i, strend); // get 32 bit code of a utf-8 symbol
            if (code == 0)
                continue;
            char* end = utf8::append(code, symbol); // initialize array `symbol`
            
            std::string word(symbol, end);

            // int index = voc.wordIndex(word);
            int index = ufind(voc, word);
            if(index>=0 && index < dim){
                vec[index]++;
            }

            if(lastword != "<sos>"){
                index = ufind(voc, lastword + " " + word);
                if(index>=0 && index < dim){
                    vec[index]++;
                }
            }
            lastword = word;
        }
        while ( str_i < strend );
    }
};

int load_vocab(std::string const& name, std::vector<std::string>& dict){
    std::fstream reader(name, std::fstream::in);
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
    // lexiTree& termDic)
    UMap& termDic)
{
    assert(IDF.size()==0);
    assert(tfdocs.size()==0);
    // assert(termDic.size==0);
    assert(termDic.empty());

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
            assert(vid_2==-1 || (vid_2 >= 0 && vid_2 < VOCAB_SZ));

            std::string term;
            if(vid_2==-1){
                term = vocab[vid_1];
            }
            else{
                term = vocab[vid_1] + " " + vocab[vid_2];
            }

            term_id = IDF.size();
            // termDic.insert(term, term_id);
            termDic[term] = term_id;

            // pivoted normalization
            // IDF.push_back(std::log((DOC_SZ+1)/(float)N));
            // Okapi
            IDF.push_back(std::max(IDF_epsilon, std::log((DOC_SZ - N + 0.5)/(N + 0.5))));
            assert(IDF.back() > 0);
        }else{

            file_id = -1;
            tf = 0;

            if(!(std::stringstream(sentence) >> file_id >> tf)) exit(1);

            assert(file_id >= 0 && file_id < DOC_SZ && tf >= 0);

            if(vid_2 == -1){
                // Unigram
                tfdocs[file_id].tid.push_back(term_id);
                tfdocs[file_id].freq.push_back(tf);
                tfdocs[file_id].length += tf * vocab[vid_1].size();
            }else{
                // Bigram
                tfdocs[file_id].tid.push_back(term_id);
                tfdocs[file_id].freq.push_back(tf);
                // tfdocs[file_id].length += tf;
            }

            N--;
        }
        
    }
}

float normalize_docs(std::vector<Document>& docs, std::vector<float>const& IDF){
    float avgdl = 0.;
    for(auto& d : docs){
        avgdl += d.length;// / (float)docs.size();
    }
    avgdl /= (float)docs.size();

    for(auto& d : docs){
        d.normalize(avgdl, IDF);
    }
    return avgdl;
}

void load_queries(
    std::string const& name,
    // lexiTree const& vocab,
    UMap const& vocab,
    std::vector<Query>& qlist)
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
    std::vector<int> const& rel)
{
    std::vector<bool> relmap(DOC_SZ, false);
    for(auto i: rel)
        relmap[i] = true;
    int dim = query.vec.size();
    std::vector<float> pos(dim, 0.);
    std::vector<float> neg(dim, 0.);

    for(int i = 0; i < DOC_SZ; i++){
        for(int j = 0; j < tfdocs[i].tid.size(); j++){
            int tid = tfdocs[i].tid[j];
            int fq = tfdocs[i].freq[j];
            if(relmap[i]){
                pos[tid] += fq / (float)rel.size();
            }else{
                neg[tid] += fq / (float)(DOC_SZ - rel.size());
            }
        }
    }
    float norm1 = 0.;
    for(int j = 0; j < dim; j++){
        norm1 += query.vec[j]*query.vec[j];
    }
    norm1 = std::sqrt(norm1);

    float norm2 = 0.;
    for(int j = 0; j < dim; j++){
        norm2 += pos[j]*pos[j];
    }
    norm2 = std::sqrt(norm2);

    float norm3 = 0.;
    for(int j = 0; j < dim; j++){
        norm3 += neg[j]*neg[j];
    }
    norm3 = std::sqrt(norm3);

    // float norm1 = 1., norm2 = 1., norm3 = 1.;

    float norm4 = 0.;
    for(int j = 0; j < dim; j++){
        query.vec[j] = R_a * query.vec[j]/norm1 + R_b * pos[j]/norm2 - R_c * neg[j]/norm3;
        norm4 += query.vec[j] * query.vec[j];
    }
    norm4 = std::sqrt(norm4);

    for(int j = 0; j < dim; j++){
        query.vec[j] = query.vec[j] * norm1 / norm4;
    }
}


void parseArgs(int argc, char** argv, 
    std::string& model_name,
    std::string& query_name,
    std::string& output_name,
    bool& best,
    bool& feedback)
{
    int ready = 0;
    for(int i = 1; i < argc; i++){
        std::string op(argv[i]);
        if(op=="-r"){
            feedback = true;
        }else if(op=="-b"){
            best = true;
        }else if(op=="-i"){
            query_name = std::string(argv[++i]);
            ready |= 1;
        }else if(op=="-o"){
            output_name = std::string(argv[++i]);
            ready |= 2;
        }else if(op=="-m"){
            model_name = std::string(argv[++i]);
            ready |= 4;
        }else if(op=="-d"){
            std::string dname = std::string(argv[++i]);
            ready |= 8;
        }else{
            std::cerr << "ERROR: Argument " + op + " not understood." << endl;
            exit(1);
        }
    }
    if(ready != 15){
        std::cerr << "ERROR: Arguments not completely specified." << endl;
        exit(1);
    }
}

int main(int argc, char** argv){
    std::string model_name;    
    std::string query_name = "query-train.xml";
    std::string output_name = "ans.csv";
    bool USE_FEEDBACK = false;
    bool USE_BEST = false;

    parseArgs(argc, argv, model_name, query_name, output_name, USE_BEST, USE_FEEDBACK);

    std::string vocab_name = model_name + "/vocab.all";
    std::string doclist_name = model_name + "/file-list";
    std::string freq_name = model_name + "/inverted-file";



    std::vector<std::string> vocab;
    std::vector<std::string> docname;
    // lexiTree bigram;
    UMap ubigram;
    

    VOCAB_SZ = load_vocab(vocab_name, vocab);
    cout << "[info] Vocab size: " << VOCAB_SZ << endl;
    
    DOC_SZ = load_filenames(doclist_name, docname);
    cout << "[info] Document list size: " << DOC_SZ << endl;

    // // load raw term frequency
    std::vector<Document> TF_docs;
    std::vector<float> IDF;

    // load_raw_TF(freq_name, vocab, TF_docs, IDF, bigram);
    load_raw_TF(freq_name, vocab, TF_docs, IDF, ubigram);
    int TERM_SZ = IDF.size();
    cout << "[info] Term size: " << TERM_SZ << endl;
    // assert(TERM_SZ==bigram.size);

    cout << "[info] Average doc length (bytes): " << normalize_docs(TF_docs, IDF) << endl;    

    std::vector<Query> QList;
    // load_queries(query_name, bigram, QList);
    load_queries(query_name, ubigram, QList);
    Kmax<float, int> maxScores(MAXCAND);


    std::fstream writer(output_name, std::fstream::out);
    writer << "query_id,retrieved_docs";

    for(int q = 0; q < QList.size(); q++){
        auto& query = QList[q];
        std::vector<int> top_candidates;

        for(int k = 0; USE_FEEDBACK && k < R_rounds; k++){
            maxScores.clear();
            for(int i = 0; i < DOC_SZ; i++){
                float score = query.match(TF_docs[i]);
                maxScores.insert(score, i);
            }        
            maxScores.extract(top_candidates);

            feedback(query, TF_docs, top_candidates);
        }
        
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

        // std::vector<float> scr(DOC_SZ);
        // std::vector<int> idx(DOC_SZ);
        // for(int i = 0; i < DOC_SZ; i++){
        //     scr[i] = query.match(TF_docs[i]);
        //     idx[i] = i;
        // }
        // std::sort(idx.begin(), idx.end(), [&scr](size_t i1, size_t i2) {return scr[i1] > scr[i2];});//ascending

        // writer << endl << query.qid << ",";
        // for(int i = 0; i < MAXCAND; i++){
        //     assert(scr[idx[i]] >= scr[idx[i+1]]);
        //     int id = idx[i];
        //     writer << docname[id] << " ";
        // }
        

        std::string bar(30, '.');
        std::fill(bar.begin(), bar.begin() + (int)std::floor(30*(q+1) / QList.size()),'=');
        std::cerr << "(" << q+1 << "/" << QList.size() << ") queries done. Progress [" << bar << "]\r";
    }
}
