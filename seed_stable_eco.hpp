/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

using namespace eosio;

using std::string;

namespace eosiosystem {
   class system_contract;
}

class seed : public contract {
   public:
      seed( account_name self ):contract(self){}
      
      /// @abi action
      void create( uint64_t seed_id,
                   asset        initial_supply);

      /// @abi action
      void transfer( account_name from,
                     account_name to,
                     asset        quantity,
                     string       memo );
   
      /// @abi action
      void growdistribute(uint64_t ratio, uint64_t seed_id);

      /// @abi action
      void burn( account_name from, asset value );

      /// @abi action
      void plant( account_name from, asset value );

      /// @abi action
      void cutdownplant( string memo );

      /// @abi action
      void release_dapp();


      inline asset get_circulate_supply( uint64_t seed_id )const;
      inline asset get_planted_supply( uint64_t seed_id )const;
      inline asset get_total_trans_volume( uint64_t seed_id )const;
      
      inline asset get_available_seeds( account_name owner )const;
      inline asset get_planted_seeds( account_name owner )const;
      inline asset get_trans_volume( account_name owner )const;

      /// @abi table accounts
      struct account {
         account_name account; 
         asset    availableseeds;
         asset    plantedseeds;
         asset    transvolume;

         uint64_t  accounttype; // 0 = normal account; 1 = local dac; 2 = eco dac; 3 = developer; 4 = burn account; 5 = plant account

         uint64_t primary_key()const { return account; }

         uint64_t get_account_type()const { return accounttype; }
      };

   private:

      /// @abi table stats
      struct seed_stats {
         uint64_t       seedid;
         asset          circulatesupply;
         asset          plantedsupply;
         asset          initialsupply;
         asset          totaltransvolume;

         uint64_t primary_key()const { return seedid; }
      };

      typedef eosio::multi_index<N(accounts), account, indexed_by< N(by_type),
      const_mem_fun<account, uint64_t, &account::get_account_type> >
      > accounts;

      typedef eosio::multi_index<N(seed_stats), seed_stats> stats;

      void sub_seed( account_name owner, asset value );
      void add_seed( account_name owner, asset value, account_name ram_payer );
      void burn_seed( account_name owner, asset value );
      void plant_seed( account_name owner, asset value );
      account findbytype( account_name owner, uint64_t type );
      account findbyaccount( account_name owner, account_name account );

   public:
      struct transfer_args {
         account_name  from;
         account_name  to;
         asset         quantity;
         string        memo;
      };
};

asset seed::get_circulate_supply( uint64_t seed_id )const
{
   stats statstable( _self, seed_id );
   const auto& st = statstable.get( seed_id );
   return st.circulatesupply;
}

asset seed::get_planted_supply( uint64_t seed_id)const
{
   stats statstable(_self, seed_id);
   const auto& st = statstable.get( seed_id );
   return st.plantedsupply;
}

asset seed::get_total_trans_volume( uint64_t seed_id)const
{
   stats statstable(_self, seed_id);
   const auto& st = statstable.get( seed_id );
   return st.totaltransvolume;
}

asset seed::get_available_seeds( account_name owner )const
{
   accounts accountstable( _self, owner );
   const auto& ac = accountstable.get( owner );
   return ac.availableseeds;
}

asset seed::get_planted_seeds( account_name owner )const
{
   accounts accountstable( _self, owner );
   const auto& ac = accountstable.get( owner );
   return ac.plantedseeds;
}

asset seed::get_trans_volume( account_name owner )const
{
   accounts accountstable( _self, owner );
   const auto& ac = accountstable.get( owner );
   return ac.transvolume;
}
