/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/database.hpp>

#include <fc/uint128.hpp>

#include <cmath>

using namespace graphene::chain;

namespace {

   void remove_trailing_zeros(std::string& str) { 
      if (str.find('.') == std::string::npos) {
         return;
      }
      
      int offset = 1; 
      if (str.find_last_not_of('0') == str.find('.')) { 
         offset = 0; 
      } 
      str.erase(str.find_last_not_of('0') + offset, std::string::npos); 
   } 

}


/*
void graphene::chain::asset_bitasset_data_object::update_median_feeds(time_point_sec current_time)
{
   current_feed_publication_time = current_time;
   vector<std::reference_wrapper<const price_feed>> current_feeds;
   for( const pair<account_id_type, pair<time_point_sec,price_feed>>& f : feeds )
   {
      if( (current_time - f.second.first).to_seconds() < options.feed_lifetime_sec &&
          f.second.first != time_point_sec() )
      {
         current_feeds.emplace_back(f.second.second);
         current_feed_publication_time = std::min(current_feed_publication_time, f.second.first);
      }
   }

   // If there are no valid feeds, or the number available is less than the minimum to calculate a median...
   if( current_feeds.size() < options.minimum_feeds )
   {
      //... don't calculate a median, and set a null feed
      current_feed_publication_time = current_time;
      current_feed = price_feed();
      return;
   }
   if( current_feeds.size() == 1 )
   {
      current_feed = std::move(current_feeds.front());
      return;
   }

   // *** Begin Median Calculations ***
   price_feed median_feed;
   const auto median_itr = current_feeds.begin() + current_feeds.size() / 2;
#define CALCULATE_MEDIAN_VALUE(r, data, field_name) \
   std::nth_element( current_feeds.begin(), median_itr, current_feeds.end(), \
                     [](const price_feed& a, const price_feed& b) { \
      return a.field_name < b.field_name; \
   }); \
   median_feed.field_name = median_itr->get().field_name;

   BOOST_PP_SEQ_FOR_EACH( CALCULATE_MEDIAN_VALUE, ~, GRAPHENE_PRICE_FEED_FIELDS )
#undef CALCULATE_MEDIAN_VALUE
   // *** End Median Calculations ***

   current_feed = median_feed;
}
*/

asset asset_object::amount_from_string(string amount_string) const
{ 

   try {
      remove_trailing_zeros(amount_string);

      bool negative_found = false;
      bool decimal_found = false;
      for( const char c : amount_string )
      {
         if( isdigit( c ) )
            continue;

         if( c == '-' && !negative_found )
         {
            negative_found = true;
            continue;
         }

         if( c == '.' && !decimal_found )
         {
            decimal_found = true;
            continue;
         }

         FC_THROW( (amount_string) );
      }

      share_type satoshis = 0;

      share_type scaled_precision = asset::scaled_precision( precision );

      const auto decimal_pos = amount_string.find( '.' );
      const string lhs = amount_string.substr( negative_found, decimal_pos );
      if( !lhs.empty() )
         satoshis += fc::safe<int64_t>(std::stoll(lhs)) *= scaled_precision;

      if( decimal_found )
      {
         const size_t max_rhs_size = std::to_string( scaled_precision.value ).substr( 1 ).size();

         string rhs = amount_string.substr( decimal_pos + 1, precision );
         FC_ASSERT( rhs.size() <= max_rhs_size );

         while( rhs.size() < max_rhs_size )
            rhs += '0';

         if( !rhs.empty() )
            satoshis += std::stoll( rhs );
      }

      FC_ASSERT( satoshis <= GRAPHENE_MAX_SHARE_SUPPLY );

      if( negative_found )
         satoshis *= -1;

      return amount(satoshis);
   } FC_CAPTURE_AND_RETHROW( (amount_string) ) 

}

string asset_object::amount_to_string(share_type amount) const
{
   share_type scaled_precision = 1;
   for( uint8_t i = 0; i < precision; ++i )
      scaled_precision *= 10;
   assert(scaled_precision > 0);

   string result = fc::to_string(amount.value / scaled_precision.value);
   auto decimals = amount.value % scaled_precision.value;
   if( decimals )
      result += "." + fc::to_string(scaled_precision.value + decimals).erase(0,1);
   return result;
}

void graphene::chain::monitored_asset_options::update_median_feeds(time_point_sec current_time)
{
   current_feed_publication_time = current_time;
   vector<std::reference_wrapper<const price_feed>> current_feeds;
   for( const pair<account_id_type, pair<time_point_sec,price_feed>>& f : feeds )
   {
      if( (current_time - f.second.first).to_seconds() < feed_lifetime_sec &&
          f.second.first != time_point_sec() )
      {
         current_feeds.emplace_back(f.second.second);
         current_feed_publication_time = std::min(current_feed_publication_time, f.second.first);
      }
   }

   // If there are no valid feeds, or the number available is less than the minimum to calculate a median...
   if( current_feeds.size() < minimum_feeds )
   {
      //... don't calculate a median, and set a null feed
      current_feed_publication_time = current_time;
      current_feed = price_feed();
      return;
   }
   if( current_feeds.size() == 1 )
   {
      current_feed = std::move(current_feeds.front());
      return;
   }

   // *** Begin Median Calculations ***
   price_feed median_feed;
   const auto median_itr = current_feeds.begin() + current_feeds.size() / 2;
#define CALCULATE_MEDIAN_VALUE(r, data, field_name) \
   std::nth_element( current_feeds.begin(), median_itr, current_feeds.end(), \
                     [](const price_feed& a, const price_feed& b) { \
      return a.field_name < b.field_name; \
   }); \
   median_feed.field_name = median_itr->get().field_name;

   BOOST_PP_SEQ_FOR_EACH( CALCULATE_MEDIAN_VALUE, ~, GRAPHENE_PRICE_FEED_FIELDS )
#undef CALCULATE_MEDIAN_VALUE
   // *** End Median Calculations ***

   current_feed = median_feed;
}

//template<class DB>
//asset asset_object::convert_uia( asset from, const DB& db )const


//template<class DB>
//asset asset_object::convert_mia( asset from, const DB& db ) const


//template<class DB>
//asset asset_object::convert( asset from, const DB& db ) const


//template<class DB>
//bool asset_object::can_convert( asset from, asset& to, const DB& db ) const
