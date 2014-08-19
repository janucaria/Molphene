#ifndef __Molphene__PDBParser__
#define __Molphene__PDBParser__

#include <string>
#include <iostream>
#include <tuple>
#include <boost/algorithm/string/trim.hpp>

#include "logger.h"
#include "Atom.h"
#include "Model.h"
#include "Molecule.h"
#include "Chain.h"
#include "Compound.h"
#include "Bond.h"

namespace molphene {
    class PDBParser {
    public:
        PDBParser();
        
        void parse(Molecule & mol, std::istream & stream);
        
    private:
        Model * currentModelPtr;
        Chain * currentChainPtr;
        Compound * currentCompoundPtr;
        
        std::string line_;
        
        inline std::string column(unsigned int start,  unsigned int end);
        
        float getReal(unsigned int start,  unsigned int end);
        
        unsigned int getInteger(unsigned int start,  unsigned int end);
        
        inline char getChar(unsigned int start);
        
        void handleATOMRecord(Molecule & mol);
        
        void handleMODELRecord(Molecule & mol);
        
        void handleCONECTRecord(Molecule & mol);
    };
}

#endif /* defined(__Molphene__PDBParser__) */
