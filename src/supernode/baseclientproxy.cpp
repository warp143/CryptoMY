// Copyright (c) 2017, The CryptoMY Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "mnemonics/electrum-words.h"
#include "common/command_line.h"
#include "baseclientproxy.h"
#include "cryptomy_defines.h"
#include "string_coding.h"
#include "common/util.h"

static const std::string scWalletCachePath("/cache/");

supernode::BaseClientProxy::BaseClientProxy()
{
}

void supernode::BaseClientProxy::Init()
{
    m_DAPIServer->ADD_DAPI_HANDLER(GetWalletBalance, rpc_command::GET_WALLET_BALANCE, BaseClientProxy);
    m_DAPIServer->ADD_DAPI_HANDLER(CreateAccount, rpc_command::CREATE_ACCOUNT, BaseClientProxy);
    m_DAPIServer->ADD_DAPI_HANDLER(GetSeed, rpc_command::GET_SEED, BaseClientProxy);
    m_DAPIServer->ADD_DAPI_HANDLER(RestoreAccount, rpc_command::RESTORE_ACCOUNT, BaseClientProxy);
}

bool supernode::BaseClientProxy::GetWalletBalance(const supernode::rpc_command::GET_WALLET_BALANCE::request &in, supernode::rpc_command::GET_WALLET_BALANCE::response &out)
{
    std::unique_ptr<tools::CryptoMyWallet> wal = initWallet(base64_decode(in.Account), in.Password);
    if (!wal)
    {
        out.Result = ERROR_OPEN_WALLET_FAILED;
        return false;
    }
    try
    {
        wal->refresh();
        out.Balance = wal->balance();
        out.UnlockedBalance = wal->unlocked_balance();
        storeWalletState(std::move(wal));
    }
    catch (const std::exception& e)
    {
        out.Result = ERROR_BALANCE_NOT_AVAILABLE;
        return false;
    }
    out.Result = STATUS_OK;
    return true;
}

bool supernode::BaseClientProxy::CreateAccount(const supernode::rpc_command::CREATE_ACCOUNT::request &in, supernode::rpc_command::CREATE_ACCOUNT::response &out)
{
    std::vector<std::string> languages;
    crypto::ElectrumWords::get_language_list(languages);
    std::vector<std::string>::iterator it;
    it = std::find(languages.begin(), languages.end(), in.Language);
    if (it == languages.end())
    {
        out.Result = ERROR_LANGUAGE_IS_NOT_FOUND;
        return false;
    }

    std::unique_ptr<tools::CryptoMyWallet> wal =
            tools::CryptoMyWallet::createWallet(std::string(), m_Servant->GetNodeIp(), m_Servant->GetNodePort(),
                                             m_Servant->GetNodeLogin(), m_Servant->IsTestnet());
    if (!wal)
    {
        out.Result = ERROR_CREATE_WALLET_FAILED;
        return false;
    }
    wal->set_seed_language(in.Language);
    wal->set_refresh_from_block_height(m_Servant->GetCurrentBlockHeight());
    crypto::secret_key dummy_key;
    try
    {
        wal->generate_cryptomy(in.Password, dummy_key, false, false);
    }
    catch (const std::exception& e)
    {
        out.Result = ERROR_CREATE_WALLET_FAILED;
        return false;
    }
    out.Account = base64_encode(wal->store_keys_cryptomy(in.Password));
    out.Address = wal->get_account().get_public_address_str(wal->testnet());
    out.ViewKey = epee::string_tools::pod_to_hex(wal->get_account().get_keys().m_view_secret_key);
    std::string seed;
    wal->get_seed(seed);
    out.Seed = seed;
    out.Result = STATUS_OK;
    return true;
}

bool supernode::BaseClientProxy::GetSeed(const supernode::rpc_command::GET_SEED::request &in, supernode::rpc_command::GET_SEED::response &out)
{
    std::unique_ptr<tools::CryptoMyWallet> wal = initWallet(base64_decode(in.Account), in.Password);
    if (!wal)
    {
        out.Result = ERROR_OPEN_WALLET_FAILED;
        return false;
    }
    wal->set_seed_language(in.Language);
    std::string seed;
    wal->get_seed(seed);
    out.Seed = seed;
    out.Result = STATUS_OK;
    return true;
}

bool supernode::BaseClientProxy::RestoreAccount(const supernode::rpc_command::RESTORE_ACCOUNT::request &in, supernode::rpc_command::RESTORE_ACCOUNT::response &out)
{
    if (in.Seed.empty())
    {
        out.Result = ERROR_ELECTRUM_SEED_EMPTY;
        return false;
    }
    crypto::secret_key recovery_key;
    std::string old_language;
    if (!crypto::ElectrumWords::words_to_bytes(in.Seed, recovery_key, old_language))
    {
        out.Result = ERROR_ELECTRUM_SEED_INVALID;
        return false;
    }
    std::unique_ptr<tools::CryptoMyWallet> wal =
        tools::CryptoMyWallet::createWallet(std::string(), m_Servant->GetNodeIp(), m_Servant->GetNodePort(),
                                             m_Servant->GetNodeLogin(), m_Servant->IsTestnet());
    if (!wal)
    {
        out.Result = ERROR_CREATE_WALLET_FAILED;
        return false;
    }
    try
    {
        wal->set_seed_language(old_language);
        wal->generate_cryptomy(in.Password, recovery_key, true, false);
        out.Account = base64_encode(wal->store_keys_cryptomy(in.Password));
        out.Address = wal->get_account().get_public_address_str(wal->testnet());
        out.ViewKey = epee::string_tools::pod_to_hex(
                    wal->get_account().get_keys().m_view_secret_key);
        std::string seed;
        wal->get_seed(seed);
        out.Seed = seed;
    }
    catch (const std::exception &e)
    {
        out.Result = ERROR_RESTORE_WALLET_FAILED;
        return false;
    }
    out.Result = STATUS_OK;
    return true;
}

std::unique_ptr<tools::CryptoMyWallet> supernode::BaseClientProxy::initWallet(const string &account, const string &password) const
{
    std::unique_ptr<tools::CryptoMyWallet> wal;
    try
    {
        wal = tools::CryptoMyWallet::createWallet(account, password, "",
                                               m_Servant->GetNodeIp(), m_Servant->GetNodePort(),
                                         m_Servant->GetNodeLogin(), m_Servant->IsTestnet());
        std::string lDataDir = tools::get_default_data_dir() + scWalletCachePath;
        if (!boost::filesystem::exists(lDataDir))
        {
            boost::filesystem::create_directories(lDataDir);
        }
        std::string lCacheFile = lDataDir +
                wal->get_account().get_public_address_str(wal->testnet());
        if (boost::filesystem::exists(lCacheFile))
        {
            wal->load_cache(lCacheFile);
        }
    }
    catch (const std::exception& e)
    {
        wal = nullptr;
    }
    return wal;
}

void supernode::BaseClientProxy::storeWalletState(std::unique_ptr<tools::CryptoMyWallet> wallet)
{
    if (wallet)
    {
        std::string lDataDir = tools::get_default_data_dir() + scWalletCachePath;
        if (!boost::filesystem::exists(lDataDir))
        {
            boost::filesystem::create_directories(lDataDir);
        }
        std::string lCacheFile = lDataDir +
                wallet->get_account().get_public_address_str(wallet->testnet());
        wallet->store_cache(lCacheFile);
    }
}

string supernode::BaseClientProxy::base64_decode(const string &encoded_data)
{
    return epee::string_encoding::base64_decode(encoded_data);
}

string supernode::BaseClientProxy::base64_encode(const string &data)
{
    return epee::string_encoding::base64_encode(data);
}
