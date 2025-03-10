/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_TESTS_DAEMON_MGMT_MANAGER_COMMON_FIXTURE_HPP
#define NFD_TESTS_DAEMON_MGMT_MANAGER_COMMON_FIXTURE_HPP

#include "mgmt/manager-base.hpp"
#include "fw/forwarder.hpp"

#include "tests/test-common.hpp"
#include "tests/key-chain-fixture.hpp"
#include "tests/daemon/global-io-fixture.hpp"

#include <ndn-cxx/security/interest-signer.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace nfd::tests {

/**
 * \brief A fixture that wraps an InterestSigner.
 */
class InterestSignerFixture : public GlobalIoTimeFixture, public KeyChainFixture
{
protected:
  InterestSignerFixture();

  /**
   * \brief Create a ControlCommand request.
   * \param commandName Command name including prefix, such as `/localhost/nfd/fib/add-nexthop`
   * \param params Command parameters
   * \param format Signed Interest format
   * \param identity Signing identity
   */
  Interest
  makeControlCommandRequest(Name commandName,
                            const ControlParameters& params = {},
                            ndn::security::SignedInterestFormat format = ndn::security::SignedInterestFormat::V03,
                            const Name& identity = DEFAULT_COMMAND_SIGNER_IDENTITY);

  /**
   * \brief Create a ControlCommand request for a Prefix Announcement.
   * \param commandName Command name including prefix, such as `/localhost/nfd/rib/announce`
   * \param prefixAnnouncement Prefix Announcement object
   * \param identity Signing identity
   *
   * Per specification, Prefix Announcements use Signed Interest v0.3 only.
   */
  Interest
  makeControlCommandRequest(Name commandName,
                            const ndn::PrefixAnnouncement& prefixAnnouncement,
                            const Name& identity = DEFAULT_COMMAND_SIGNER_IDENTITY);

protected:
  static inline const Name DEFAULT_COMMAND_SIGNER_IDENTITY{"/InterestSignerFixture-identity"};

private:
  ndn::security::InterestSigner m_signer{m_keyChain};
};

/**
 * @brief A collection of common functions shared by all manager's test fixtures.
 */
class ManagerCommonFixture : public InterestSignerFixture
{
public:
  /**
   * @brief Add `/localhost/nfd` as a top prefix to the dispatcher.
   *
   * Afterwards, advanceClocks() is called to ensure all added filters take effect.
   */
  void
  setTopPrefix();

  /**
   * @brief Cause management to receive an Interest.
   *
   * Calls DummyClientFace::receive() to receive the Interest and then calls advanceClocks()
   * to ensure the Interest is dispatched.
   */
  void
  receiveInterest(const Interest& interest);

  static ControlResponse
  makeResponse(uint32_t code, const std::string& text, const ControlParameters& parameters)
  {
    return ControlResponse(code, text).setBody(parameters.wireEncode());
  }

  enum class CheckResponseResult {
    OK,
    OUT_OF_BOUNDARY,
    WRONG_NAME,
    WRONG_CONTENT_TYPE,
    INVALID_RESPONSE,
    WRONG_CODE,
    WRONG_TEXT,
    WRONG_BODY_SIZE,
    WRONG_BODY_VALUE,
  };

  /**
   * @brief Check a specified response data with the expected ControlResponse.
   *
   * @param idx the index of the specified Data in m_responses
   * @param expectedName the expected name of this Data
   * @param expectedResponse the expected ControlResponse
   * @param expectedContentType the expected content type of this Data, use -1 to skip this check
   *
   * @retval OK the response at the specified index can be decoded from the response data,
   *            and its code, text and response body are all matched with the expected response
   * @retval OUT_OF_BOUNDARY the specified index out of boundary
   * @retval WRONG_NAME the name of the specified response data does not match
   * @retval WRONG_CONTENT_TYPE the content type of the specified response data does not match
   * @retval INVALID_RESPONSE the data name matches but it fails in decoding a ControlResponse from
   *                          the content of the specified response data
   * @retval WRONG_CODE a valid ControlResponse can be decoded but has a wrong code
   * @retval WRONG_TEXT a valid ControlResponse can be decoded but has a wrong text
   * @retval WRONG_BODY_SIZE the body size of decoded ControlResponse does not match
   * @retval WRONT_BODY_VALUE the body value of decoded ControlResponse does not match
   */
  CheckResponseResult
  checkResponse(size_t idx,
                const Name& expectedName,
                const ControlResponse& expectedResponse,
                int expectedContentType = -1);

  /**
   * @brief Concatenate specified response Data into a single block.
   *
   * @param startIndex the start index in m_responses
   * @param nResponses the number of response to concatenate
   *
   * @return the generated block
   */
  Block
  concatenateResponses(size_t startIndex = 0, size_t nResponses = 0);

protected:
  ndn::DummyClientFace m_face{g_io, m_keyChain, {true, true}};
  Dispatcher m_dispatcher{m_face, m_keyChain};
  std::vector<Data>& m_responses{m_face.sentData};
};

std::ostream&
operator<<(std::ostream& os, ManagerCommonFixture::CheckResponseResult result);

class ManagerFixtureWithAuthenticator : public ManagerCommonFixture
{
public:
  /**
   * \brief Grant m_identityName privilege to sign commands for the management module.
   */
  void
  setPrivilege(const std::string& privilege);

protected:
  FaceTable m_faceTable;
  Forwarder m_forwarder{m_faceTable};
  shared_ptr<CommandAuthenticator> m_authenticator = CommandAuthenticator::create();
};

class CommandSuccess
{
public:
  static ControlResponse
  getExpected()
  {
    return ControlResponse()
        .setCode(200)
        .setText("OK");
  }
};

template<auto CODE>
class CommandFailure
{
public:
  static ControlResponse
  getExpected()
  {
    return ControlResponse()
        .setCode(CODE);
    // error description should not be checked
  }
};

} // namespace nfd::tests

#endif // NFD_TESTS_DAEMON_MGMT_MANAGER_COMMON_FIXTURE_HPP
