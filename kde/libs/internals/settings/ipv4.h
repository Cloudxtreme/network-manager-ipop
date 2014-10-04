// This file is generated by kconfig_compiler from ipv4.kcfg.
// All changes you do to this file will be lost.
#ifndef KNM_IPV4SETTING_H
#define KNM_IPV4SETTING_H

#include <QHostAddress>
#include <kglobal.h>
#include <kdebug.h>
#include <kcoreconfigskeleton.h>
#include <solid/control/networkipv4confignm09.h>
#include "setting.h"
#include "knminternals_export.h"

Q_DECLARE_METATYPE(Solid::Control::IPv4AddressNm09)
Q_DECLARE_METATYPE(Solid::Control::IPv4RouteNm09)

namespace Knm {

class KNMINTERNALS_EXPORT Ipv4Setting : public Setting
{
  public:
    class EnumMethod
    {
      public:
      enum type { Automatic, LinkLocal, Manual, Shared, Disabled, COUNT };
    };

    Ipv4Setting( );
    Ipv4Setting(Ipv4Setting *);
    ~Ipv4Setting();

    QString name() const;

    bool hasSecrets() const;

    /**
      Set Method
    */
    void setMethod( int v )
    {
        mMethod = v;
    }

    /**
      Get Method
    */
    int method() const
    {
      return mMethod;
    }

    /**
      Set DNS Servers
    */
    void setDns( const QList<QHostAddress> & v )
    {
        mDns = v;
    }

    /**
      Get DNS Servers
    */
    QList<QHostAddress> dns() const
    {
      return mDns;
    }

    /**
      Set Search Domains
    */
    void setDnssearch( const QStringList & v )
    {
        mDnssearch = v;
    }

    /**
      Get Search Domains
    */
    QStringList dnssearch() const
    {
      return mDnssearch;
    }

    /**
      Set IP Addresses
    */
    void setAddresses( const QList<Solid::Control::IPv4AddressNm09> & v )
    {
        mAddresses = v;
    }

    /**
      Get IP Addresses
    */
    QList<Solid::Control::IPv4AddressNm09> addresses() const
    {
      return mAddresses;
    }

    /**
      Set Ignore DHCP DNS
    */
    void setIgnoredhcpdns( bool v )
    {
        mIgnoredhcpdns = v;
    }

    /**
      Get Ignore DHCP DNS
    */
    bool ignoredhcpdns() const
    {
      return mIgnoredhcpdns;
    }

    /**
      Set Ignore Automatic Routes
    */
    void setIgnoreautoroute( bool v )
    {
        mIgnoreautoroute = v;
    }

    /**
      Get Ignore Automatic Routes
    */
    bool ignoreautoroute() const
    {
      return mIgnoreautoroute;
    }

    /**
      Set Never Default Route
    */
    void setNeverdefault( bool v )
    {
        mNeverdefault = v;
    }

    /**
      Get Never Default Route
    */
    bool neverdefault() const
    {
      return mNeverdefault;
    }

    /**
      Set DHCP Client ID
    */
    void setDhcpclientid( const QString & v )
    {
        mDhcpclientid = v;
    }

    /**
      Get DHCP Client ID
    */
    QString dhcpclientid() const
    {
      return mDhcpclientid;
    }

    /**
      Set DHCP hostname
    */
    void setDhcphostname( const QString & v )
    {
        mDhcphostname = v;
    }

    /**
      Get DHCP hostname
    */
    QString dhcphostname() const
    {
      return mDhcphostname;
    }

    QList<Solid::Control::IPv4RouteNm09> routes() const
    {
        return mRoutes;
    }

    void setRoutes(QList<Solid::Control::IPv4RouteNm09> routes)
    {
        mRoutes = routes;
    }

    /**
    Set May Fail
    */
    void setMayfail( bool v )
    {
        mMayfail = v;
    }

    /**
      Get May Fail
    */
    bool mayfail() const
    {
      return mMayfail;
    }


  protected:

    // ipv4
    int mMethod;
    QList<QHostAddress> mDns;
    QStringList mDnssearch;
    QList<Solid::Control::IPv4AddressNm09> mAddresses;
    QList<Solid::Control::IPv4RouteNm09> mRoutes;
    bool mIgnoredhcpdns;
    bool mIgnoreautoroute;
    bool mNeverdefault;
    QString mDhcpclientid;
    QString mDhcphostname;
    bool mMayfail;
  private:
};

}

#endif
