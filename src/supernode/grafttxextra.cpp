#include "cryptomytxextra.h"
#include <ios>

namespace supernode {


CryptoMyTxExtra::CryptoMyTxExtra(uint64_t _blocknum, const std::string &_payment_id, const std::vector<std::string> _signs)
  : BlockNum(_blocknum)
  , PaymentID(_payment_id)
  , Signs(_signs)
{

}

bool CryptoMyTxExtra::operator ==(const CryptoMyTxExtra &rhs) const
{
    return this->BlockNum == rhs.BlockNum
            && this->PaymentID == rhs.PaymentID
            && this->Signs == rhs.Signs;
}

}

