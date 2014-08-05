#pragma once
#include <bts/blockchain/types.hpp>
#include <bts/blockchain/withdraw_types.hpp>
#include <bts/blockchain/transaction.hpp>

namespace bts { namespace blockchain {
    
    struct dice_record
    {
        dice_record(){}
        
        dice_record( const withdraw_condition& c )
        :condition(c){}
        
        address owner()const;
        
        bool is_null()const;
        
        dice_record make_null()const;
        
        dice_id_type        id = dice_id_type();
        withdraw_condition  condition;
        share_type          amount;
        uint32_t            odds;
    };
    
    struct jackpot_transaction
    {
        jackpot_transaction(){}
        
        address                                   play_owner;
        address                                   jackpot_owner;
        share_type                                play_amount;
        share_type                                jackpot_received;
        uint32_t                                  odds;
        uint32_t                                  lucky_number;
    };
    
    typedef fc::optional<dice_record> odice_record;
    
} } // bts::blockchain

FC_REFLECT( bts::blockchain::dice_record,
           (id)(condition)(amount)(odds)
           )
FC_REFLECT_TYPENAME( std::vector<bts::blockchain::jackpot_transaction> )
FC_REFLECT( bts::blockchain::jackpot_transaction,
           (play_owner)
           (jackpot_owner)
           (play_amount)
           (jackpot_received)
           (odds)
           (lucky_number)
           )