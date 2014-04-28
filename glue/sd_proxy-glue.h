
/*
 *	This file was automatically generated by dbusxx-xml2cpp; DO NOT EDIT!
 */

#ifndef __dbusxx__glue_sd_proxy_glue_h__PROXY_MARSHAL_H
#define __dbusxx__glue_sd_proxy_glue_h__PROXY_MARSHAL_H

#include <dbus-c++/dbus.h>
#include <cassert>

namespace com {
namespace bluegiga {
namespace v2 {
namespace bt {

class sd_proxy
: public ::DBus::InterfaceProxy
{
public:

    sd_proxy()
    : ::DBus::InterfaceProxy("com.bluegiga.v2.bt.sd")
    {
        connect_signal(sd_proxy, SearchResultInd, _SearchResultInd_stub);
        connect_signal(sd_proxy, CloseSearchInd, _CloseSearchInd_stub);
    }

public:

    /* properties exported by this interface */
public:

    /* methods exported by this interface,
     * this functions will invoke the corresponding methods on the remote objects
     */
    void SearchReq(const uint16_t& phandle, const uint32_t& searchConfig, const uint32_t& rssiBufferTime, const uint32_t& totalSearchTime, const int16_t& rssiThreshold, const uint32_t& deviceClass, const uint32_t& deviceClassMask, const uint32_t& inquiryAccessCode, const std::vector< uint8_t >& filter)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << phandle;
        wi << searchConfig;
        wi << rssiBufferTime;
        wi << totalSearchTime;
        wi << rssiThreshold;
        wi << deviceClass;
        wi << deviceClassMask;
        wi << inquiryAccessCode;
        wi << filter;
        call.member("SearchReq");
        ::DBus::Message ret = invoke_method (call);
    }

    void CancelSearchReq(const uint16_t& phandle)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << phandle;
        call.member("CancelSearchReq");
        ::DBus::Message ret = invoke_method (call);
    }


public:

    /* signal handlers for this interface
     */
    virtual void SearchResultInd(const std::string& deviceAddr, const uint32_t& deviceClass, const int16_t& rssi, const std::map< uint16_t, ::DBus::Variant >& info, const uint32_t& deviceStatus) = 0;
    virtual void CloseSearchInd(const uint16_t& resultCode, const uint16_t& resultSupplier) = 0;

private:

    /* unmarshalers (to unpack the DBus message before calling the actual signal handler)
     */
    void _SearchResultInd_stub(const ::DBus::SignalMessage &sig)
    {
        ::DBus::MessageIter ri = sig.reader();

        std::string deviceAddr;
        ri >> deviceAddr;
        uint32_t deviceClass;
        ri >> deviceClass;
        int16_t rssi;
        ri >> rssi;
        std::map< uint16_t, ::DBus::Variant > info;
        ri >> info;
        uint32_t deviceStatus;
        ri >> deviceStatus;
        SearchResultInd(deviceAddr, deviceClass, rssi, info, deviceStatus);
    }
    void _CloseSearchInd_stub(const ::DBus::SignalMessage &sig)
    {
        ::DBus::MessageIter ri = sig.reader();

        uint16_t resultCode;
        ri >> resultCode;
        uint16_t resultSupplier;
        ri >> resultSupplier;
        CloseSearchInd(resultCode, resultSupplier);
    }
};  /* class sd_proxy */

} } } } 
#endif //__dbusxx__glue_sd_proxy_glue_h__PROXY_MARSHAL_H
