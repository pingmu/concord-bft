// Concord
//
// Copyright (c) 2020 VMware, Inc. All Rights Reserved.
//
// This product is licensed to you under the Apache 2.0 license (the "License"). You may not use this product except in
// compliance with the Apache 2.0 License.
//
// This product may include a number of subcomponents with separate copyright notices and license terms. Your use of
// these subcomponents is subject to the terms and conditions of the sub-component's license, as noted in the LICENSE
// file.

#pragma once

#include <memory>
#include "threshsign/ThresholdSignaturesTypes.h"
#include "threshsign/IThresholdSigner.h"
#include "threshsign/IThresholdVerifier.h"
#include "ReplicaConfig.hpp"

namespace bftEngine {

class CryptoManager {
 public:
  static CryptoManager& instance(const ReplicaConfig* config = nullptr, Cryptosystem* cryptoSys = nullptr) {
    static CryptoManager cm_(config, cryptoSys);
    return cm_;
  }

  IThresholdSigner* thresholdSignerForSlowPathCommit() const { return thresholdSigner_.get(); }
  IThresholdVerifier* thresholdVerifierForSlowPathCommit() const { return thresholdVerifierForSlowPathCommit_.get(); }

  IThresholdSigner* thresholdSignerForCommit() const { return thresholdSigner_.get(); }
  IThresholdVerifier* thresholdVerifierForCommit() const { return thresholdVerifierForCommit_.get(); }

  IThresholdSigner* thresholdSignerForOptimisticCommit() const { return thresholdSigner_.get(); }
  IThresholdVerifier* thresholdVerifierForOptimisticCommit() const {
    return thresholdVerifierForOptimisticCommit_.get();
  }

  std::pair<std::string, std::string> generateMultisigKeyPair() { return multiSigCryptoSystem_->generateNewKeyPair(); }

 private:
  CryptoManager(const ReplicaConfig* config, Cryptosystem* cryptoSys)
      : f_{config->fVal}, c_{config->cVal}, numSigners_{config->numReplicas} {
    multiSigCryptoSystem_.reset(cryptoSys);
    init();
  }

  void init() {
    thresholdSigner_.reset(multiSigCryptoSystem_->createThresholdSigner());
    thresholdVerifierForSlowPathCommit_.reset(multiSigCryptoSystem_->createThresholdVerifier(f_ * 2 + c_ + 1));
    thresholdVerifierForCommit_.reset(multiSigCryptoSystem_->createThresholdVerifier(f_ * 3 + c_ + 1));
    thresholdVerifierForOptimisticCommit_.reset(multiSigCryptoSystem_->createThresholdVerifier(numSigners_));
  }

  void updateMultisigKeys(const std::string& secretKey, const std::string& verificationKey) {
    multiSigCryptoSystem_->updateKeys(secretKey, verificationKey);
    init();
  }

  void updateVerificationKeyForSigner(const std::string& verificationKey, const std::uint16_t& signerIndex) {
    multiSigCryptoSystem_->updateVerificationKey(verificationKey, signerIndex);
    init();
  }

  CryptoManager(const CryptoManager&) = delete;
  CryptoManager(const CryptoManager&&) = delete;
  CryptoManager& operator=(const CryptoManager&) = delete;
  CryptoManager& operator=(const CryptoManager&&) = delete;

  std::uint16_t f_;
  std::uint16_t c_;
  std::uint16_t numSigners_;

  std::unique_ptr<Cryptosystem> multiSigCryptoSystem_;

  std::unique_ptr<IThresholdSigner> thresholdSigner_;
  // signer and verifier of a threshold signature (for threshold N-fVal-cVal out of N)
  std::unique_ptr<IThresholdVerifier> thresholdVerifierForSlowPathCommit_;

  // verifier of a threshold signature (for threshold N-cVal out of N)
  // If cVal==0, then should be nullptr
  std::unique_ptr<IThresholdVerifier> thresholdVerifierForCommit_;

  // verifier of a threshold signature (for threshold N out of N)
  std::unique_ptr<IThresholdVerifier> thresholdVerifierForOptimisticCommit_;
};
}  // namespace bftEngine