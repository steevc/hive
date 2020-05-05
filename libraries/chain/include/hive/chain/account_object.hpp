#pragma once
#include <hive/chain/steem_fwd.hpp>

#include <fc/fixed_string.hpp>

#include <hive/protocol/authority.hpp>
#include <hive/protocol/steem_operations.hpp>

#include <hive/chain/steem_object_types.hpp>
#include <hive/chain/witness_objects.hpp>
#include <hive/chain/shared_authority.hpp>
#include <hive/chain/util/manabar.hpp>

#include <hive/chain/util/delayed_voting_processor.hpp>
#include <hive/chain/util/greedy_asset.hpp>

#include <numeric>

namespace hive { namespace chain {

   using chainbase::t_vector;
   using hive::protocol::authority;

   class account_object : public object< account_object_type, account_object >
   {
         STEEM_STD_ALLOCATOR_CONSTRUCTOR( account_object )

         template<typename Constructor, typename Allocator>
         account_object( Constructor&& c, allocator< Allocator > a )
         : delayed_votes( a )
         {
            c(*this);
         }

         //liquid HIVE balance
         asset get_balance() const { return balance; }
         //HIVE balance in savings
         asset get_savings() const { return savings_balance; }
         //unclaimed HIVE rewards
         asset get_rewards() const { return reward_steem_balance; }

         //liquid HBD balance
         asset get_hbd_balance() const { return sbd_balance; }
         //HBD balance in savings
         asset get_hbd_savings() const { return savings_sbd_balance; }
         //unclaimed HBD rewards
         asset get_hbd_rewards() const { return reward_sbd_balance; }

         //all VESTS held by the account - use other routines to get active VESTS for specific uses
         asset get_vesting() const { return vesting_shares; }
         //VESTS that were delegated to other accounts
         asset get_delegated_vesting() const { return delegated_vesting_shares; }
         //VESTS that were borrowed from other accounts
         asset get_received_vesting() const { return received_vesting_shares; }
         //TODO: add routines for specific uses, f.e. get_witness_voting_power, get_proposal_voting_power, get_post_voting_power...
         //unclaimed VESTS rewards
         asset get_vest_rewards() const { return reward_vesting_balance; }
         //value of unclaimed VESTS rewards in HIVE (HIVE held on global balance)
         asset get_vest_rewards_as_hive() const { return reward_vesting_steem; }

         id_type           id;

         account_name_type name;
         public_key_type   memo_key;
         account_name_type proxy;

         time_point_sec    last_account_update;

         time_point_sec    created;
         bool              mined = true;
         account_name_type recovery_account;
         account_name_type reset_account = HIVE_NULL_ACCOUNT;
         time_point_sec    last_account_recovery;
         uint32_t          comment_count = 0;
         uint32_t          lifetime_vote_count = 0;
         uint32_t          post_count = 0;

         bool              can_vote = true;
         util::manabar     voting_manabar;
         util::manabar     downvote_manabar;

         greedy_STEEM_asset balance = asset( 0, HIVE_SYMBOL );  ///< total liquid shares held by this account
         greedy_STEEM_asset savings_balance = asset( 0, HIVE_SYMBOL );  ///< total liquid shares held by this account

         /**
          *  HBD Deposits pay interest based upon the interest rate set by witnesses. The purpose of these
          *  fields is to track the total (time * sbd_balance) that it is held. Then at the appointed time
          *  interest can be paid using the following equation:
          *
          *  interest = interest_rate * sbd_seconds / seconds_per_year
          *
          *  Every time the sbd_balance is updated the sbd_seconds is also updated. If at least
          *  STEEM_MIN_COMPOUNDING_INTERVAL_SECONDS has past since sbd_last_interest_payment then
          *  interest is added to sbd_balance.
          *
          *  @defgroup sbd_data sbd Balance Data
          */
         ///@{
         greedy_SBD_asset  sbd_balance = asset( 0, HBD_SYMBOL ); /// total sbd balance
         uint128_t         sbd_seconds; ///< total sbd * how long it has been hel
         time_point_sec    sbd_seconds_last_update; ///< the last time the sbd_seconds was updated
         time_point_sec    sbd_last_interest_payment; ///< used to pay interest at most once per month


         greedy_SBD_asset  savings_sbd_balance = asset( 0, HBD_SYMBOL ); /// total sbd balance
         uint128_t         savings_sbd_seconds; ///< total sbd * how long it has been hel
         time_point_sec    savings_sbd_seconds_last_update; ///< the last time the sbd_seconds was updated
         time_point_sec    savings_sbd_last_interest_payment; ///< used to pay interest at most once per month

         uint8_t           savings_withdraw_requests = 0;
         ///@}

         greedy_SBD_asset   reward_sbd_balance = asset( 0, HBD_SYMBOL );
         greedy_STEEM_asset reward_steem_balance = asset( 0, HIVE_SYMBOL );
         greedy_VEST_asset  reward_vesting_balance = asset( 0, VESTS_SYMBOL );
         greedy_STEEM_asset reward_vesting_steem = asset( 0, HIVE_SYMBOL );

         share_type        curation_rewards = 0;
         share_type        posting_rewards = 0;

         greedy_VEST_asset vesting_shares = asset( 0, VESTS_SYMBOL ); ///< total vesting shares held by this account, controls its voting power
         greedy_VEST_asset delegated_vesting_shares = asset( 0, VESTS_SYMBOL );
         greedy_VEST_asset received_vesting_shares = asset( 0, VESTS_SYMBOL );

         greedy_VEST_asset vesting_withdraw_rate = asset( 0, VESTS_SYMBOL ); ///< at the time this is updated it can be at most vesting_shares/104
         time_point_sec    next_vesting_withdrawal = fc::time_point_sec::maximum(); ///< after every withdrawal this is incremented by 1 week
         share_type        withdrawn = 0; /// Track how many shares have been withdrawn
         share_type        to_withdraw = 0; /// Might be able to look this up with operation history.
         uint16_t          withdraw_routes = 0;

         fc::array<share_type, HIVE_MAX_PROXY_RECURSION_DEPTH> proxied_vsf_votes;// = std::vector<share_type>( HIVE_MAX_PROXY_RECURSION_DEPTH, 0 ); ///< the total VFS votes proxied to this account

         uint16_t          witnesses_voted_for = 0;

         time_point_sec    last_post;
         time_point_sec    last_root_post = fc::time_point_sec::min();
         time_point_sec    last_post_edit;
         time_point_sec    last_vote_time;
         uint32_t          post_bandwidth = 0;

         share_type        pending_claimed_accounts = 0;

         using t_delayed_votes = t_vector< delayed_votes_data >;
         /*
            Holds sum of VESTS per day.
            VESTS from day `X` will be matured after `X` + 30 days ( because `HIVE_DELAYED_VOTING_TOTAL_INTERVAL_SECONDS` == 30 days )
         */
         t_delayed_votes   delayed_votes;
         /*
            Total sum of VESTS from `delayed_votes` collection.
            It's a helper variable needed for better performance.
         */
         ushare_type       sum_delayed_votes = 0;

         time_point_sec get_the_earliest_time() const
         {
            if( !delayed_votes.empty() )
               return ( delayed_votes.begin() )->time;
            else
               return time_point_sec::maximum();
         }

         share_type get_real_vesting_shares() const
         {
            FC_ASSERT( sum_delayed_votes.value <= vesting_shares.amount, "",
                        ( "sum_delayed_votes",     sum_delayed_votes )
                        ( "vesting_shares.amount", vesting_shares.amount )
                        ( "account",               name ) );
  
            return asset( vesting_shares.amount - sum_delayed_votes.value, VESTS_SYMBOL ).amount;
         }

         /// This function should be used only when the account votes for a witness directly
         share_type        witness_vote_weight()const {
            return std::accumulate( proxied_vsf_votes.begin(),
                                    proxied_vsf_votes.end(),
                                    get_real_vesting_shares()
                                  );
         }
         share_type        proxied_vsf_votes_total()const {
            return std::accumulate( proxied_vsf_votes.begin(),
                                    proxied_vsf_votes.end(),
                                    share_type() );
         }
   };

   class account_metadata_object : public object< account_metadata_object_type, account_metadata_object >
   {
      STEEM_STD_ALLOCATOR_CONSTRUCTOR( account_metadata_object )

      template< typename Constructor, typename Allocator >
      account_metadata_object( Constructor&& c, allocator< Allocator > a )
         : json_metadata( a ), posting_json_metadata( a )
      {
         c( *this );
      }

      id_type           id;
      account_id_type   account;
      shared_string     json_metadata;
      shared_string     posting_json_metadata;
   };

   class account_authority_object : public object< account_authority_object_type, account_authority_object >
   {
      STEEM_STD_ALLOCATOR_CONSTRUCTOR( account_authority_object )

      public:
         template< typename Constructor, typename Allocator >
         account_authority_object( Constructor&& c, allocator< Allocator > a )
            : owner( a ), active( a ), posting( a )
         {
            c( *this );
         }

         id_type           id;

         account_name_type account;

         shared_authority  owner;   ///< used for backup control, can set owner or active
         shared_authority  active;  ///< used for all monetary operations, can set active or posting
         shared_authority  posting; ///< used for voting and posting

         time_point_sec    last_owner_update;
   };

   class vesting_delegation_object : public object< vesting_delegation_object_type, vesting_delegation_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         vesting_delegation_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         vesting_delegation_object() {}

         //amount of delegated VESTS
         const asset& get_vesting() const { return vesting_shares; }

         id_type           id;
         account_name_type delegator;
         account_name_type delegatee;
         asset             vesting_shares;
         time_point_sec    min_delegation_time;
   };

   class vesting_delegation_expiration_object : public object< vesting_delegation_expiration_object_type, vesting_delegation_expiration_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         vesting_delegation_expiration_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         vesting_delegation_expiration_object() {}

         //amount of expiring delegated VESTS
         const asset& get_vesting() const { return vesting_shares; }

         id_type           id;
         account_name_type delegator;
         asset             vesting_shares;
         time_point_sec    expiration;
   };

   class owner_authority_history_object : public object< owner_authority_history_object_type, owner_authority_history_object >
   {
      STEEM_STD_ALLOCATOR_CONSTRUCTOR( owner_authority_history_object )

      public:
         template< typename Constructor, typename Allocator >
         owner_authority_history_object( Constructor&& c, allocator< Allocator > a )
            :previous_owner_authority( allocator< shared_authority >( a ) )
         {
            c( *this );
         }

         id_type           id;

         account_name_type account;
         shared_authority  previous_owner_authority;
         time_point_sec    last_valid_time;
   };

   class account_recovery_request_object : public object< account_recovery_request_object_type, account_recovery_request_object >
   {
      STEEM_STD_ALLOCATOR_CONSTRUCTOR( account_recovery_request_object )

      public:
         template< typename Constructor, typename Allocator >
         account_recovery_request_object( Constructor&& c, allocator< Allocator > a )
            :new_owner_authority( allocator< shared_authority >( a ) )
         {
            c( *this );
         }

         id_type           id;

         account_name_type account_to_recover;
         shared_authority  new_owner_authority;
         time_point_sec    expires;
   };

   class change_recovery_account_request_object : public object< change_recovery_account_request_object_type, change_recovery_account_request_object >
   {
      STEEM_STD_ALLOCATOR_CONSTRUCTOR( change_recovery_account_request_object )

      public:
         template< typename Constructor, typename Allocator >
         change_recovery_account_request_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type           id;

         account_name_type account_to_recover;
         account_name_type recovery_account;
         time_point_sec    effective_on;
   };

   struct by_proxy;
   struct by_next_vesting_withdrawal;
   struct by_delayed_voting;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< account_object, account_id_type, &account_object::id > >,
         ordered_unique< tag< by_name >,
            member< account_object, account_name_type, &account_object::name > >,
         ordered_unique< tag< by_proxy >,
            composite_key< account_object,
               member< account_object, account_name_type, &account_object::proxy >,
               member< account_object, account_name_type, &account_object::name >
            > /// composite key by proxy
         >,
         ordered_unique< tag< by_next_vesting_withdrawal >,
            composite_key< account_object,
               member< account_object, time_point_sec, &account_object::next_vesting_withdrawal >,
               member< account_object, account_name_type, &account_object::name >
            > /// composite key by_next_vesting_withdrawal
         >,
         ordered_unique< tag< by_delayed_voting >,
            composite_key< account_object,
               const_mem_fun< account_object, time_point_sec, &account_object::get_the_earliest_time >,
               member< account_object, account_id_type, &account_object::id >
            >
         >
      >,
      allocator< account_object >
   > account_index;

   struct by_account;

   typedef multi_index_container <
      account_metadata_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< account_metadata_object, account_metadata_id_type, &account_metadata_object::id > >,
         ordered_unique< tag< by_account >,
            member< account_metadata_object, account_id_type, &account_metadata_object::account > >
      >,
      allocator< account_metadata_object >
   > account_metadata_index;

   typedef multi_index_container <
      owner_authority_history_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< owner_authority_history_object, owner_authority_history_id_type, &owner_authority_history_object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< owner_authority_history_object,
               member< owner_authority_history_object, account_name_type, &owner_authority_history_object::account >,
               member< owner_authority_history_object, time_point_sec, &owner_authority_history_object::last_valid_time >,
               member< owner_authority_history_object, owner_authority_history_id_type, &owner_authority_history_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< time_point_sec >, std::less< owner_authority_history_id_type > >
         >
      >,
      allocator< owner_authority_history_object >
   > owner_authority_history_index;

   struct by_last_owner_update;

   typedef multi_index_container <
      account_authority_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< account_authority_object, account_authority_id_type, &account_authority_object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< account_authority_object,
               member< account_authority_object, account_name_type, &account_authority_object::account >,
               member< account_authority_object, account_authority_id_type, &account_authority_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< account_authority_id_type > >
         >,
         ordered_unique< tag< by_last_owner_update >,
            composite_key< account_authority_object,
               member< account_authority_object, time_point_sec, &account_authority_object::last_owner_update >,
               member< account_authority_object, account_authority_id_type, &account_authority_object::id >
            >,
            composite_key_compare< std::greater< time_point_sec >, std::less< account_authority_id_type > >
         >
      >,
      allocator< account_authority_object >
   > account_authority_index;

   struct by_delegation;

   typedef multi_index_container <
      vesting_delegation_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< vesting_delegation_object, vesting_delegation_id_type, &vesting_delegation_object::id > >,
         ordered_unique< tag< by_delegation >,
            composite_key< vesting_delegation_object,
               member< vesting_delegation_object, account_name_type, &vesting_delegation_object::delegator >,
               member< vesting_delegation_object, account_name_type, &vesting_delegation_object::delegatee >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
         >
      >,
      allocator< vesting_delegation_object >
   > vesting_delegation_index;

   struct by_expiration;
   struct by_account_expiration;

   typedef multi_index_container <
      vesting_delegation_expiration_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< vesting_delegation_expiration_object, vesting_delegation_expiration_id_type, &vesting_delegation_expiration_object::id > >,
         ordered_unique< tag< by_expiration >,
            composite_key< vesting_delegation_expiration_object,
               member< vesting_delegation_expiration_object, time_point_sec, &vesting_delegation_expiration_object::expiration >,
               member< vesting_delegation_expiration_object, vesting_delegation_expiration_id_type, &vesting_delegation_expiration_object::id >
            >,
            composite_key_compare< std::less< time_point_sec >, std::less< vesting_delegation_expiration_id_type > >
         >,
         ordered_unique< tag< by_account_expiration >,
            composite_key< vesting_delegation_expiration_object,
               member< vesting_delegation_expiration_object, account_name_type, &vesting_delegation_expiration_object::delegator >,
               member< vesting_delegation_expiration_object, time_point_sec, &vesting_delegation_expiration_object::expiration >,
               member< vesting_delegation_expiration_object, vesting_delegation_expiration_id_type, &vesting_delegation_expiration_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< time_point_sec >, std::less< vesting_delegation_expiration_id_type > >
         >
      >,
      allocator< vesting_delegation_expiration_object >
   > vesting_delegation_expiration_index;

   struct by_expiration;

   typedef multi_index_container <
      account_recovery_request_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< account_recovery_request_object, account_recovery_request_id_type, &account_recovery_request_object::id > >,
         ordered_unique< tag< by_account >,
            member< account_recovery_request_object, account_name_type, &account_recovery_request_object::account_to_recover >
         >,
         ordered_unique< tag< by_expiration >,
            composite_key< account_recovery_request_object,
               member< account_recovery_request_object, time_point_sec, &account_recovery_request_object::expires >,
               member< account_recovery_request_object, account_name_type, &account_recovery_request_object::account_to_recover >
            >,
            composite_key_compare< std::less< time_point_sec >, std::less< account_name_type > >
         >
      >,
      allocator< account_recovery_request_object >
   > account_recovery_request_index;

   struct by_effective_date;

   typedef multi_index_container <
      change_recovery_account_request_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< change_recovery_account_request_object, change_recovery_account_request_id_type, &change_recovery_account_request_object::id > >,
         ordered_unique< tag< by_account >,
            member< change_recovery_account_request_object, account_name_type, &change_recovery_account_request_object::account_to_recover >
         >,
         ordered_unique< tag< by_effective_date >,
            composite_key< change_recovery_account_request_object,
               member< change_recovery_account_request_object, time_point_sec, &change_recovery_account_request_object::effective_on >,
               member< change_recovery_account_request_object, account_name_type, &change_recovery_account_request_object::account_to_recover >
            >,
            composite_key_compare< std::less< time_point_sec >, std::less< account_name_type > >
         >
      >,
      allocator< change_recovery_account_request_object >
   > change_recovery_account_request_index;
} }

#ifdef ENABLE_MIRA
namespace mira {

template<> struct is_static_length< hive::chain::vesting_delegation_object > : public boost::true_type {};
template<> struct is_static_length< hive::chain::vesting_delegation_expiration_object > : public boost::true_type {};
template<> struct is_static_length< hive::chain::change_recovery_account_request_object > : public boost::true_type {};

} // mira
#endif

FC_REFLECT( hive::chain::account_object,
             (id)(name)(memo_key)(proxy)(last_account_update)
             (created)(mined)
             (recovery_account)(last_account_recovery)(reset_account)
             (comment_count)(lifetime_vote_count)(post_count)(can_vote)(voting_manabar)(downvote_manabar)
             (balance)
             (savings_balance)
             (sbd_balance)(sbd_seconds)(sbd_seconds_last_update)(sbd_last_interest_payment)
             (savings_sbd_balance)(savings_sbd_seconds)(savings_sbd_seconds_last_update)(savings_sbd_last_interest_payment)(savings_withdraw_requests)
             (reward_steem_balance)(reward_sbd_balance)(reward_vesting_balance)(reward_vesting_steem)
             (vesting_shares)(delegated_vesting_shares)(received_vesting_shares)
             (vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
             (curation_rewards)
             (posting_rewards)
             (proxied_vsf_votes)(witnesses_voted_for)
             (last_post)(last_root_post)(last_post_edit)(last_vote_time)(post_bandwidth)
             (pending_claimed_accounts)
             (delayed_votes)
             (sum_delayed_votes)
          )

CHAINBASE_SET_INDEX_TYPE( hive::chain::account_object, hive::chain::account_index )

FC_REFLECT( hive::chain::account_metadata_object,
             (id)(account)(json_metadata)(posting_json_metadata) )
CHAINBASE_SET_INDEX_TYPE( hive::chain::account_metadata_object, hive::chain::account_metadata_index )

FC_REFLECT( hive::chain::account_authority_object,
             (id)(account)(owner)(active)(posting)(last_owner_update)
)
CHAINBASE_SET_INDEX_TYPE( hive::chain::account_authority_object, hive::chain::account_authority_index )

FC_REFLECT( hive::chain::vesting_delegation_object,
            (id)(delegator)(delegatee)(vesting_shares)(min_delegation_time) )
CHAINBASE_SET_INDEX_TYPE( hive::chain::vesting_delegation_object, hive::chain::vesting_delegation_index )

FC_REFLECT( hive::chain::vesting_delegation_expiration_object,
            (id)(delegator)(vesting_shares)(expiration) )
CHAINBASE_SET_INDEX_TYPE( hive::chain::vesting_delegation_expiration_object, hive::chain::vesting_delegation_expiration_index )

FC_REFLECT( hive::chain::owner_authority_history_object,
             (id)(account)(previous_owner_authority)(last_valid_time)
          )
CHAINBASE_SET_INDEX_TYPE( hive::chain::owner_authority_history_object, hive::chain::owner_authority_history_index )

FC_REFLECT( hive::chain::account_recovery_request_object,
             (id)(account_to_recover)(new_owner_authority)(expires)
          )
CHAINBASE_SET_INDEX_TYPE( hive::chain::account_recovery_request_object, hive::chain::account_recovery_request_index )

FC_REFLECT( hive::chain::change_recovery_account_request_object,
             (id)(account_to_recover)(recovery_account)(effective_on)
          )
CHAINBASE_SET_INDEX_TYPE( hive::chain::change_recovery_account_request_object, hive::chain::change_recovery_account_request_index )

namespace helpers
{
   template <>
   class index_statistic_provider<hive::chain::account_index>
   {
   public:
      typedef hive::chain::account_index IndexType;
      typedef typename hive::chain::account_object::t_delayed_votes t_delayed_votes;

      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);

         if(onlyStaticInfo == false)
         {
            for(const auto& o : index)
            {
               info._item_additional_allocation += o.delayed_votes.capacity()*sizeof(t_delayed_votes::value_type);
            }
         }

         return info;
      }
   };

} /// namespace helpers