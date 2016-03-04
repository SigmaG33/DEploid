/*
 * pfDeconv is used for deconvoluting Plasmodium falciparum genome from
 * mix-infected patient sample.
 *
 * Copyright (C) 2016, Sha (Joe) Zhu, Jacob Almagro and Prof. Gil McVean
 *
 * This file is part of pfDeconv.
 *
 * scrm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "atMarker.hpp"
#include "pfDeconvIO.hpp"
#include <cassert>       // assert
#include <iomanip>      // std::setw


PfDeconvIO::~PfDeconvIO(){}


PfDeconvIO::PfDeconvIO(int argc, char *argv[]) {
    argv_ = std::vector<std::string>(argv + 1, argv + argc);
    this->argv_i = argv_.begin();

    this->init();
    if ( argv_.size() == 0 ) {
        this->set_help(true);
        return;
    }

    this->parse();
    this->checkInput();
    this->finalize();
}


void PfDeconvIO::init() {
    this->seed_set_ = false;
    this->set_seed( 0 );
    this->set_help(false);
    this->set_panel(true);
    this->precision_ = 8;
    this->prefix_ = "pf3k-pfDeconv";
    this->kStrain_ = 5;
    this->nMcmcSample_ = 1000;
    this->mcmcMachineryRate_ = 5;

    #ifdef COMPILEDATE
        compileTime_ = COMPILEDATE;
    #else
        compileTime_ = "";
    #endif

    #ifdef PFDECONVVERSION
        pfDeconvVersion_ = PFDECONVVERSION;
    #else
        pfDeconvVersion_ = "";
    #endif
}


void PfDeconvIO::finalize(){
    AtMarker ref (refFileName_.c_str());
    this->refCount_ = ref.info_;

    AtMarker alt (altFileName_.c_str());
    this->altCount_ = alt.info_;

    AtMarker plaf (plafFileName_.c_str());
    this->plaf_ = plaf.info_;
    this->chrom_ = plaf.chrom_;
    this->position_ = plaf.position_;

    assert( indexOfChromStarts_.size() == 0 );
    indexOfChromStarts_.push_back( (size_t) 0);

    for ( ; indexOfChromStarts_.size() < this->chrom_.size(); ){
        indexOfChromStarts_.push_back(indexOfChromStarts_.back()+this->position_[indexOfChromStarts_.size()].size());
    }
    assert( indexOfChromStarts_.size() == this->chrom_.size() );

    this->nLoci_ = refCount_.size();
    assert( this->nLoci_ == this->plaf_.size() );
    assert( this->altCount_.size() == this->nLoci_ );
    (void)removeFilesWithSameName();
}


void PfDeconvIO::removeFilesWithSameName(){
    strExportLLK = this->prefix_ + ".llk";
    strExportHap = this->prefix_ + ".hap";
    strExportProp = this->prefix_ + ".prop";
    strExportLog =  this->prefix_ + ".log";

    remove(strExportLLK.c_str());
    remove(strExportHap.c_str());
    remove(strExportProp.c_str());
    remove(strExportLog.c_str());
}


void PfDeconvIO::parse (){

    do {
        if (*argv_i == "-ref") {
            this->readNextStringto ( this->refFileName_ ) ;
        } else if (*argv_i == "-alt") {
            this->readNextStringto ( this->altFileName_ ) ;
        } else if (*argv_i == "-plaf") {
            this->readNextStringto ( this->plafFileName_ ) ;
        } else if (*argv_i == "-panel") {
            if ( this->usePanel() == false ){
                throw ("use panel?");
            }
            this->readNextStringto ( this->panelFileName_ ) ;
        } else if (*argv_i == "-noPanel"){
            if ( usePanel() && this->panelFileName_.size() > 0 ){
                throw ("use panel?");
            }
            this->set_panel(false);
        } else if (*argv_i == "-o") {
            this->readNextStringto ( this->prefix_ ) ;
        } else if ( *argv_i == "-p" ) {
            this->precision_ = readNextInput<size_t>() ;
        } else if ( *argv_i == "-k" ) {
            this->kStrain_ = readNextInput<size_t>() ;
        } else if ( *argv_i == "-nSample" ) {
            this->nMcmcSample_ = readNextInput<size_t>() ;
        } else if ( *argv_i == "-rate" ) {
            this->mcmcMachineryRate_ = readNextInput<size_t>() ;
        } else if (*argv_i == "-seed"){
            this->random_seed_ = readNextInput<size_t>() ;
            this->seed_set_ = true;
        } else if (*argv_i == "-h" || *argv_i == "-help"){
            this->set_help(true);
        } else {
            throw ( UnknowArg((*argv_i)) );
        }
    } while ( ++argv_i != argv_.end());
}


void PfDeconvIO::checkInput(){
    if ( this->refFileName_.size() == 0 )
        throw FileNameMissing ( "Ref count" );
    if ( this->altFileName_.size() == 0 )
        throw FileNameMissing ( "Alt count" );
    if ( this->plafFileName_.size() == 0 )
        throw FileNameMissing ( "PLAF" );
    if ( usePanel() && this->panelFileName_.size() == 0 )
        throw FileNameMissing ( "Reference panel" );
}


void PfDeconvIO::readNextStringto( string &readto ){
    string tmpFlag = *argv_i;
    ++argv_i;
    if (argv_i == argv_.end() || (*argv_i)[0] == '-' )
        throw NotEnoughArg(tmpFlag);
    readto = *argv_i;
}


void PfDeconvIO::printHelp(){
    cout << endl
         << "pfDeconv " << VERSION
         << endl
         << endl;
    cout << "Usage:"
         << endl;
    cout << setw(20) << "-h or -help"         << "  --  " << "Help. List the following content."<<endl;
    cout << setw(20) << "-ref STR"            << "  --  " << "Path of reference allele count file."<<endl;
    cout << setw(20) << "-alt STR"            << "  --  " << "Path of alternative allele count file."<<endl;
    cout << setw(20) << "-plaf STR"           << "  --  " << "Path of population level allele frequency file."<<endl;
    cout << setw(20) << "-panel STR"          << "  --  " << "Path of reference panel."<<endl;
    cout << setw(20) << "-o STR"              << "  --  " << "Specify the file name prefix of the output."<<endl;
    cout << setw(20) << "-p INT"              << "  --  " << "Out put precision (default value 8)."<<endl;
    cout << setw(20) << "-k INT"              << "  --  " << "Number of strain (default value 5)."<<endl;
    cout << setw(20) << "-seed INT"           << "  --  " << "Random seed."<<endl;
    cout << setw(20) << "-nSample INT"        << "  --  " << "Number of MCMC samples."<<endl;
    cout << setw(20) << "-rate INT"           << "  --  " << "MCMC sample rate."<<endl;
    cout << endl;
    cout << "Examples:" << endl;
    cout << endl;
    cout << "./pfDeconv -ref labStrains/PG0390_first100ref.txt -alt labStrains/PG0390_first100alt.txt -plaf labStrains/labStrains_first100_PLAF.txt -panel labStrains/lab_first100_Panel.txt -o tmp1" << endl;
    cout << "./pfDeconv -ref labStrains/PG0390_first100ref.txt -alt labStrains/PG0390_first100alt.txt -plaf labStrains/labStrains_first100_PLAF.txt -panel labStrains/lab_first100_Panel.txt -nSample 100 -rate 3" << endl;
    cout << "./pfDeconv_dbg -ref labStrains/PG0390_first100ref.txt -alt labStrains/PG0390_first100alt.txt -plaf labStrains/labStrains_first100_PLAF.txt -panel labStrains/lab_first100_Panel.txt -nSample 100 -rate 3" << endl;
    cout << "./pfDeconv_dbg -ref labStrains/PG0390.C_ref.txt -alt labStrains/PG0390.C_alt.txt -plaf labStrains/labStrains_samples_PLAF.txt -panel labStrains/clonalPanel.csv -nSample 500 -rate 5" << endl;
}
