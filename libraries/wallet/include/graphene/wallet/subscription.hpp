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

#ifndef DECENT_WALLET_SUBSCRIPTION_H
#define DECENT_WALLET_SUBSCRIPTION_H

/**
 * @brief Creates a subscription to author. This function is used by consumers.
 * @param from Account who wants subscription to author
 * @param to The author you wish to subscribe to
 * @param price_amount Price for the subscription
 * @param price_asset_symbol Ticker symbol of the asset which will be used to buy subscription ( must be DCT token )
 * @param broadcast True if you wish to broadcast the transaction
 * @return The signed transaction subscribing the consumer to the author
 * @ingroup WalletCLI
 */
signed_transaction subscribe_to_author( string from,
                                        string to,
                                        string price_amount,
                                        string price_asset_symbol,
                                        bool broadcast/* = false */);

/**
 * @brief Creates a subscription to author. This function is used by author.
 * @param from The account obtaining subscription from the author
 * @param to The name or id of the author
 * @param broadcast True if you wish to broadcast the transaction
 * @return The signed transaction subscribing the consumer to the author
 * @ingroup WalletCLI
 */
signed_transaction subscribe_by_author( string from,
                                        string to,
                                        bool broadcast/* = false */);

/**
 * @brief This function can be used to allow/disallow subscription.
 * @param account The name or id of the account to update
 * @param allow_subscription True if account (author) wants to allow subscription, false otherwise
 * @param subscription_period Duration of subscription in days
 * @param price_amount Price for subscription per one subscription period
 * @param price_asset_symbol Ticker symbol of the asset which will be used to buy subscription
 * @param broadcast True if you wish to broadcast the transaction
 * @return The signed transaction updating the account
 * @ingroup WalletCLI
 */
signed_transaction set_subscription( string account,
                                     bool allow_subscription,
                                     uint32_t subscription_period,
                                     string price_amount,
                                     string price_asset_symbol,
                                     bool broadcast/* = false */);

/**
 * @brief This function can be used to allow/disallow automatic renewal of expired subscription.
 * @param account_id_or_name The name or id of the account to update
 * @param subscription_id The ID of the subscription.
 * @param automatic_renewal True if account (consumer) wants to allow automatic renewal of subscription, false otherwise
 * @param broadcast True if you wish to broadcast the transaction
 * @return The signed transaction allowing/disallowing renewal of the subscription
 * @ingroup WalletCLI
 */
signed_transaction set_automatic_renewal_of_subscription( string account_id_or_name,
                                                          subscription_id_type subscription_id,
                                                          bool automatic_renewal,
                                                          bool broadcast/* = false */);

/**
 * @brief Get a list of consumer's active (not expired) subscriptions.
 * @param account_id_or_name The name or id of the consumer
 * @param count Maximum number of subscriptions to fetch (must not exceed 100)
 * @return List of active subscription objects corresponding to the provided consumer
 * @ingroup WalletCLI
 */
vector< subscription_object > list_active_subscriptions_by_consumer( const string& account_id_or_name, const uint32_t count)const;

/**
 * @brief Get a list of consumer's subscriptions.
 * @param account_id_or_name The name or id of the consumer
 * @param count Maximum number of subscriptions to fetch (must not exceed 100)
 * @return List of subscription objects corresponding to the provided consumer
 * @ingroup WalletCLI
 */
vector< subscription_object > list_subscriptions_by_consumer( const string& account_id_or_name, const uint32_t count)const;

/**
 * @brief Get a list of active (not expired) subscriptions to author.
 * @param account_id_or_name The name or id of the author
 * @param count Maximum number of subscriptions to fetch (must not exceed 100)
 * @return List of active subscription objects corresponding to the provided author
 * @ingroup WalletCLI
 */
vector< subscription_object > list_active_subscriptions_by_author( const string& account_id_or_name, const uint32_t count)const;

/**
 * @brief Get a list of subscriptions to author.
 * @param account_id_or_name The name or id of the author
 * @param count Maximum number of subscriptions to fetch (must not exceed 100)
 * @return List of subscription objects corresponding to the provided author
 * @ingroup WalletCLI
 */
vector< subscription_object > list_subscriptions_by_author( const string& account_id_or_name, const uint32_t count)const;


#endif //DECENT_WALLET_SUBSCRIPTION_H