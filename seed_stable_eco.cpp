#include <seed_stable_eco.hpp>

/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

using namespace eosio;

void seed::create( uint64_t seed_id, asset initial_supply)
{
    require_auth( _self );

    auto sym = initial_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( initial_supply.is_valid(), "invalid supply");
    eosio_assert( initial_supply.amount > 0, "initial-supply must be positive");

    stats statstable( _self, seed_id );
    auto existing = statstable.find( seed_id);
    eosio_assert( existing == statstable.end(), "seed with seed_id already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.circulatesupply.symbol = initial_supply.symbol;
       s.plantedsupply.symbol = initial_supply.symbol;
       s.initialsupply = initial_supply;
       s.seedid        = seed_id;
    });
}

void seed::growdistrib(uint64_t ratio, uint64_t seed_id)
{
    stats statstable( _self, seed_id );
    auto existing = statstable.find( seed_id);

    asset grow_seeds = existing->totaltransvolume * ratio / 100 -  (existing->circulatesupply + existing->plantedsupply);

    printf("the grow seed amount is %lld", grow_seeds.amount);

/// to analyze
    // auto sym = quantity.symbol;
    // eosio_assert( sym.is_valid(), "invalid symbol name" );
    // eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    // auto sym_name = sym.name();
    // stats statstable( _self, sym_name );
    // auto existing = statstable.find( sym_name );
    // eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    // const auto& st = *existing;

    // require_auth( st.issuer );
    // eosio_assert( quantity.is_valid(), "invalid quantity" );
    // eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    // eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    // eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    // statstable.modify( st, 0, [&]( auto& s ) {
    //    s.supply += quantity;
    // });

    // add_balance( st.issuer, quantity, st.issuer );

    // if( to != st.issuer ) {
    //    SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
    // }

}

void seed::transfer( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.initialsupply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );


    sub_seed( from, quantity );
    add_seed( to, quantity, from );
}

void seed::burn( account_name from, asset value )
{
    require_auth( from );

    accounts burn_acnts( _self, from);
    auto typeinx = burn_acnts.get_index<N(by_type)>();
    auto item = typeinx.find(4);
    if( item == typeinx.end() ) {
        print_f("Not found type: 4");
    } else {
        print_f("Found account type:%d, %s:%\n", item->accounttype, item->account);

        burn_seed( from, value );
        add_seed( item->account, value, from );
    };


}

void seed::plant( account_name from, asset value )
{
    require_auth( from );
    accounts plant_acnts( _self, from );

    auto typeinx = plant_acnts.get_index<N(by_type)>();
    auto item = typeinx.find(5);
    if( item == typeinx.end() ) {
        print_f("Not found type: 5");
    } else {
        print_f("Found account type:%d, %s:%\n", item->accounttype, item->account);
        
        plant_seed( from, value );
        add_seed( item->account, value, from);
    };
}

void seed::cutdownplant( string memo )
{
    print("Items sorted by primary key:\n");
    accounts accountable(_self, _self);

    for( const auto& item : accountable ) {
      print(" available seeds =", item.availableseeds, 
        ", planted seeds=", item.plantedseeds, ", trans volume=", item.transvolume, "\n");
    }
}

void seed::release_dapp()
{   
    // create dapp burn 100 SEEDS
    burn( _self, (asset) eosio::asset(100, S(4, SEEDS)));
}

void seed::burn_seed( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( owner, "no seed object found" );
   eosio_assert( from.availableseeds.amount >= value.amount, "overdrawn seed" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
      a.availableseeds -= value;
   });
}

void seed::plant_seed( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( owner, "no seed object found" );
   eosio_assert( from.availableseeds.amount >= value.amount, "overdrawn seed" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
      a.availableseeds -= value;
      a.plantedseeds   += value;
   });
}

void seed::sub_seed( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( owner, "no seed object found" );
   eosio_assert( from.availableseeds.amount >= value.amount, "overdrawn seed" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
      a.availableseeds -= value;
   });
}

void seed::add_seed( account_name owner, asset value, account_name ram_payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.availableseeds = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.availableseeds += value;
      });
   }
}


EOSIO_ABI( seed, (create)(growdistrib)(burn)(plant)(cutdownplant)(transfer) )
