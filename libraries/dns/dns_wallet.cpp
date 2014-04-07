#include <bts/dns/dns_wallet.hpp>

namespace bts { namespace dns {

dns_wallet::dns_wallet()
{
}

dns_wallet::~dns_wallet()
{
}

signed_transaction dns_wallet::bid_on_domain(const std::string &name, const asset &bid_price,
                                             const signed_transactions &tx_pool, dns_db &db)
{ try {
    FC_ASSERT(is_valid_name(name), "Invalid name");

    /* Name should be new, for auction, or expired */
    bool new_or_expired;
    output_reference prev_tx_ref;
    FC_ASSERT(name_is_available(name, tx_pool, db, new_or_expired, prev_tx_ref), "Name not available");

    /* Build domain output */
    auto domain_output = claim_domain_output();
    domain_output.name = name;
    domain_output.value = std::vector<char>();
    domain_output.owner = new_recv_address("Owner address for domain: " + name);
    domain_output.last_tx_type = claim_domain_output::bid_or_auction;

    signed_transaction tx;
    std::unordered_set<address> req_sigs;

    if (new_or_expired)
    {
        tx.outputs.push_back(trx_output(domain_output, bid_price));
    }
    else
    {
        tx.inputs.push_back(trx_input(prev_tx_ref));

        auto prev_output = get_tx_ref_output(prev_tx_ref, db);
        asset transfer_amount;
        FC_ASSERT(is_valid_bid_price(prev_output.amount, bid_price, transfer_amount), "Invalid bid price");

        /* Fee is implicit from difference */
        auto prev_dns_output = to_dns_output(prev_output);
        tx.outputs.push_back(trx_output(claim_by_signature_output(prev_dns_output.owner), transfer_amount));
        tx.outputs.push_back(trx_output(domain_output, bid_price));
    }

    return collect_inputs_and_sign(tx, bid_price, req_sigs);

} FC_RETHROW_EXCEPTIONS(warn, "bid_on_domain ${name} with ${amt}", ("name", name)("amt", bid_price)) }

signed_transaction dns_wallet::update_domain_record(const std::string &name, const fc::variant &value,
                                                    const signed_transactions &tx_pool, dns_db &db)
{ try {
    FC_ASSERT(is_valid_name(name), "Invalid name");
    FC_ASSERT(is_valid_value(value), "Invalid value");

    /* Build domain output */
    auto domain_output = claim_domain_output();
    domain_output.name = name;
    domain_output.value = serialize_value(value);
    domain_output.last_tx_type = claim_domain_output::update;

    return update_or_auction_domain(true, domain_output, asset(), tx_pool, db);

} FC_RETHROW_EXCEPTIONS(warn, "update_domain_record ${name} with value ${val}", ("name", name)("val", value)) }

signed_transaction dns_wallet::auction_domain(const std::string &name, const asset &ask_price,
                                              const signed_transactions &tx_pool, dns_db &db)
{ try {
    FC_ASSERT(is_valid_name(name), "Invalid name");

    /* Build domain output */
    auto domain_output = claim_domain_output();
    domain_output.name = name;
    domain_output.value = std::vector<char>();
    domain_output.last_tx_type = claim_domain_output::bid_or_auction;

    return update_or_auction_domain(false, domain_output, ask_price, tx_pool, db);

} FC_RETHROW_EXCEPTIONS(warn, "auction_domain ${name} with ${amt}", ("name", name)("amt", ask_price)) }

signed_transaction dns_wallet::update_or_auction_domain(bool update, claim_domain_output &domain_output,
                                                        asset amount, const signed_transactions &tx_pool,
                                                        dns_db &db)
{ try {
    /* Name should exist and be owned */
    output_reference prev_tx_ref;
    FC_ASSERT(name_is_useable(domain_output.name, tx_pool, db, get_unspent_outputs(), prev_tx_ref),
            "Name unavailable for update or auction");
    auto prev_output = get_tx_ref_output(prev_tx_ref, db);

    auto prev_domain_output = to_dns_output(prev_output);
    domain_output.owner = prev_domain_output.owner;

    if (update)
        amount = prev_output.amount;

    signed_transaction tx;
    std::unordered_set<address> req_sigs;

    tx.inputs.push_back(trx_input(prev_tx_ref));
    tx.outputs.push_back(trx_output(domain_output, amount));
    req_sigs.insert(prev_domain_output.owner);

    return collect_inputs_and_sign(tx, asset(), req_sigs);

} FC_RETHROW_EXCEPTIONS(warn, "update_or_auction_domain ${name} with ${amt}", ("name", domain_output.name)("amt", amount)) }

bool dns_wallet::scan_output(transaction_state &state, const trx_output &out, const output_reference &ref,
                             const output_index &idx)
{
    if (!is_dns_output(out))
        return wallet::scan_output(state, out, ref, idx);

    auto domain_output = to_dns_output(out);

    if (is_my_address(domain_output.owner))
    {
        cache_output(out, ref, idx);
        return true;
    }

    return false;
}

} } // bts::dns
