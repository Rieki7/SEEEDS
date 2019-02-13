
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

using std::string;
using namespace eosio;

CONTRACT seeedstoken : public contract
{
    using contract::contract;

  public:

    ACTION create ();

    ACTION issue (name to, asset quantity, string memo);

    ACTION transfer (name from, name to, asset quantity, string memo);

    ACTION stake (name account, asset stake);

    ACTION unstake (name account, asset stake);

    ACTION harvest (asset seeds_to_harvest);

  private:

    const string    SEEDS_SYMBOL = "SEEDS";
    const uint8_t   SEEDS_PRECISION = 4;

    TABLE account
    {
        asset       balance;
        asset       staked_balance;
        uint64_t    period_trx_volume;
        uint64_t    period_trx_count;
        uint64_t    period_trx_partners;
        uint64_t    period_reputation_score;
        uint64_t    primary_key() const { return balance.symbol.code().raw(); }
    };

    TABLE currency_stats
    {
        asset       supply;
        asset       total_staked;
        uint64_t    period_trx_volume;
        uint64_t    period_trx_count;
        uint64_t    period_trx_partners;
        uint64_t    period_reputation_scores;
        name        issuer;
        uint64_t    primary_key() const { return supply.symbol.code().raw(); }
    };

    typedef eosio::multi_index<"accounts"_n, account> accounts;
    typedef eosio::multi_index<"stat"_n, currency_stats> stats;

    void sub_balance(name owner, asset value);
    void add_balance(name owner, asset value, name ram_payer);
};
