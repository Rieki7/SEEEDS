#include "seeedstoken.hpp"


ACTION seeedstoken::stake (name account, asset stake) 
{
    accounts a_t (get_self(), account.value);
    auto a_itr = a_t.find (stake.symbol.code().raw());
    eosio_assert (a_itr != a_t.end(), "Balance not found.");

    sub_balance(account, stake);

    a_t.modify (a_itr, eosio::same_payer, [&](auto &a) {
        a.staked_balance += stake;
    });
}

ACTION seeedstoken::unstake (name account, asset stake) 
{
    accounts a_t (get_self(), account.value);
    auto a_itr = a_t.find (stake.symbol.code().raw());
    eosio_assert (a_itr != a_t.end(), "Balance not found.");

    eosio_assert (a_itr->staked_balance >= stake, "Staked amount is less than unstake attempt.");

    a_t.modify (a_itr, eosio::same_payer, [&](auto &a) {
        a.staked_balance -= stake;
        a.balance += stake;
    });
}

ACTION seeedstoken::harvest (asset seeds_to_harvest)
{

}


ACTION seeedstoken::create()
{
    require_auth(get_self());

    auto sym = symbol { symbol_code (SEEDS_SYMBOL.c_str()), SEEDS_PRECISION};
    eosio_assert(sym.is_valid(), "invalid symbol name");
    
    stats statstable(_self, sym.code().raw());
    auto existing = statstable.find(sym.code().raw());
    eosio_assert(existing == statstable.end(), "seeedstoken with symbol already exists");

    statstable.emplace(_self, [&](auto &s) {
        s.supply.symbol = sym;
        s.issuer = get_self();
    });
}

ACTION seeedstoken::issue(name to, asset quantity, string memo)
{
    auto sym = quantity.symbol;
    eosio_assert(sym.is_valid(), "invalid symbol name");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    auto sym_name = sym.code().raw();
    stats statstable(_self, sym_name);
    auto existing = statstable.find(sym_name);
    eosio_assert(existing != statstable.end(), "seeedstoken with symbol does not exist, create seeedstoken before issue");
    const auto &st = *existing;
    require_auth(st.issuer);
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must issue positive quantity");

    eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

    statstable.modify(st, eosio::same_payer, [&](auto &s) {
        s.supply += quantity;
    });

    add_balance(st.issuer, quantity, st.issuer);

    if (to != st.issuer)
    {
        SEND_INLINE_ACTION(*this, transfer, {st.issuer, "active"_n}, {st.issuer, to, quantity, memo});
    }
}

ACTION seeedstoken::transfer(name from, name to, asset quantity, string memo)
{
    eosio_assert(from != to, "cannot transfer to self");
    require_auth(from);
    eosio_assert(is_account(to), "to account does not exist");
    auto sym = quantity.symbol.code().raw();
    stats statstable(_self, sym);
    const auto &st = statstable.get(sym);

    require_recipient(from);
    require_recipient(to);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    auto payer = has_auth(to) ? to : from;

    sub_balance(from, quantity);
    add_balance(to, quantity, payer);
}

void seeedstoken::sub_balance(name owner, asset value)
{
    accounts from_acnts(_self, owner.value);

    const auto &from = from_acnts.get(value.symbol.code().raw(), "no balance object found");
    eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

    from_acnts.modify(from, owner, [&](auto &a) {
        a.balance -= value;
    });
}

void seeedstoken::add_balance(name owner, asset value, name ram_payer)
{
    accounts to_acnts(_self, owner.value);
    auto to = to_acnts.find(value.symbol.code().raw());
    if (to == to_acnts.end())
    {
        to_acnts.emplace(ram_payer, [&](auto &a) {
            a.balance = value;
        });
    }
    else
    {
        to_acnts.modify(to, eosio::same_payer, [&](auto &a) {
            a.balance += value;
        });
    }
}

EOSIO_DISPATCH(seeedstoken, (create)(issue)(transfer)(stake)(unstake)(harvest))