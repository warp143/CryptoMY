#ifndef CRYPTOMYTXEXTRA_H
#define CRYPTOMYTXEXTRA_H

#include "cryptonote_basic/account.h"

namespace supernode {

struct CryptoMyTxExtra
{
    uint64_t BlockNum;
    std::string PaymentID;
    std::vector<std::string> Signs;

    CryptoMyTxExtra() = default;
    CryptoMyTxExtra(uint64_t _blocknum, const std::string &_payment_id, const std::vector<std::string> _signs);
    bool operator ==(const CryptoMyTxExtra &rhs) const;


    BEGIN_SERIALIZE_OBJECT()
        FIELD(BlockNum)
        FIELD(PaymentID)
        FIELD(Signs)
    END_SERIALIZE()
};

}


#endif // CRYPTOMYTXEXTRA_H
