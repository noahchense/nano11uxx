/*
 ===============================================================================
 Name        : ble_serial.h
 Author      : uCXpresso
 Version     : 1.0.19
 Date		 : 2014/9/30
 Copyright   : Copyright (C) www.embeda.com.tw
 Description : UART (Serial Stream) service for BLE
 ===============================================================================
 History
 ---------+---------+--------------------------------------------+-------------
 DATE     |	VERSION |	DESCRIPTIONS							 |	By
 ---------+---------+--------------------------------------------+-------------
 2014/1/1	v1.0.0	Initialize											Jason
 2014/1/4	v1.0.1	Add onAckTimeout() event for ACK timeout.			Jason
 2014/1/18	v1.0.2	Rename to bleSerial Class							Jason
 2014/1/19	v1.0.3	Add onAlert / onLinkLose / setTxPowerLevel			Jason
 2014/1/20	v1.0.4	Modify ble engine process for RTOS					Jason
 2014/1/22	v1.0.5	Add parameter 'stack' for enable() member			Jason
 	 	 	 	 	Add available() member to UART service check.
 2014/2/05	v1.0.6	Add isReadyForNotify() member for notify service.
 2014/2/11	v1.0.7	Add setup() member function.						Jason
 2014/2/18	v1.0.8	Add m_semDataCredit semaphore.						Jason
 2014/2/21	v1.0.9	Add connInterval & connTimeout parameters in		Jason
 	 	 	 	 	setup() member function.
 2014/2/24	v1.0.10 Rename setup() to advertising() 					Jason
 2014/3/2	v1.0.11 Add onError event to indicate an error occurred.	Jason
 	 	 	 	 	Move setTxPowerLevel member to bleProximity class.
 2014/3/8	v1.0.12 Rename member onLoseLink to onLinkLoss				Jason
 	 	 	 	 	Rename member onHrControl to onHeartRateControl
 2014/3/15	v1.0.13 Change BLE polling to interrupt method.				Jason
 2014/4/10	v1.0.14 Remove isActived() member for power save features.	Jason
 2014/6/14	v1.0.15	Update manufactureData to 24 bits					Jason
 2014/7/23	v1.0.16	Add sleep & weakup member functions.				Jason
 2014/7/25	v1.0.17 Add wait() member to block and wait a BLE event.	Jason
 2014/8/13	v1.0.18	Add Configurable FiFo Size for BLE serial.			Jason
 2014/9/30	v1.0.19 Add txdFlowControl member to enable/disable the TXD Jason
 	 	 	 	 	Flow-Control.
 2014/11/8	v2.0.0	Use Nordic UART over BLE service replaced RedBearLab Jason
 ===============================================================================
 */

#ifndef BLE_SERIAL_H
#define BLE_SERIAL_H

#include "class/stream.h"
#include "class/thread.h"
#include "class/spi.h"
#include "class/pin.h"
#include "class/semaphore.h"
#include "class/mutex.h"
#include "class/timeout.h"

/**
 * \defgroup BLE
 * BLE is meant the Bluetooth Low Energy
 */

#define BLE_RESPONSE_TIMEOUT	3000
#define DEF_BLE_DEVICENAME		"uCXpresso.BLE"
#define DEF_BLE_ADV_INTERVAL	500		// default advertising intervel 500ms
#define DEF_BLE_TXPOWER			-65		// default tx power level -65dbm @ 1M
#define DEF_BLE_NOTIFY_TIMEOUT	3000
#define DEF_BLE_DATA_TIMEOUT	1500
#define DEF_BLE_CONN_INTERVAL	100		// default connection interval 100ms
#define DEF_BLE_CONN_TIMEOUT	3000	// default connection timeout 3sec
#define DEF_BLE_MFG_DATA		0x123456

#define SUPPORT_BLE_PROXIMITY	0

/**Disconnect a BLE connection with a reason.
 * \ingroup Enumerations
 */
typedef enum {
	BLE_TERMINATED =1,		///< Request termination of the connection with the peer device with the reason "Remote user terminated connection"
	BLE_UNACCEPTABLE = 2	///< Request termination of the link with the peer device with the reason "Unacceptable connection timing"
} BLE_DISCONNECT_REASON_T;

/**setTxPower sets the output power level of the Bluetooth Low Energy radio.
 * \ingroup Enumerations
 */
typedef enum {
	BLE_TX_m18dBm 	= 0,	///< -18dBm	(Low)
	BLE_TX_m12dBm 	= 1,	///< -12dBm
	BLE_TX_m6dBm	= 2,	///< -6dBm
	BLE_TX_0dBm 	= 3		///<  0dBm	(High)
}BLE_TX_POWER_T;

/**BLE error code
 * \ingroup Enumerations
 */
typedef enum {
	BLE_ERR_OK	= 0,
	BLE_ERR_HW,				///< Hardware Error
	BLE_ERR_BUF,			///< Sender queue buffer was full
	BLE_ERR_ACK_TIMEOUT,	///< ACK timeout
	BLE_ERR_RSP_TIMEOUT,	///< Response timeout
	BLE_ERR_DAT_TIMEOUT		///< Core data timeout
}BLE_ERR_T;

#define DEF_BLE_FIFO_SIZE	32

/**\class bleSerial ble_serial.h "class/ble_serial.h"
 * \brief bleSerial class is a ble core, and inherits from CStream class to provide the stream virtual functions for serial input and output.
 * the bleSerial class also inherits from the CThread class and can be work in background.
 * \ingroup BLE
 * \remark 1.The bleSerial will use an interrupt resource, so remain 7 interrupts can be used.<br/>
 * 		   2.The bleSerial will become to a Weakup Source for Power Save Features.
 */
class bleSerial: public CStream, public CThread {
public:
	/**bleSerial constructor
	 * \brief bleSerial constructor with a GAT device name.
	 * \code
	 * int main() {
	 * 		...
	 * 		bleSerial ble("myBLE");
	 * 		ble.advertising(100);	// set advertising interval 100ms
	 * 		ble.enable();
	 * 		...
	 * 		...
	 * \endcode
	 * \param deviceName point to a LPCTSTR string to indicate
	 * the GAT device name of Bluetooth.
	 */
	bleSerial(LPCTSTR deviceName=DEF_BLE_DEVICENAME,
			  size_t tx_fifo_size=DEF_BLE_FIFO_SIZE,
			  size_t rx_fifo_size=DEF_BLE_FIFO_SIZE);

	/**Broadcast the advertising message when device is not in BLE connection.
	 * \param advInterval To broadcast the advertising message with the interval time in millisecond.
	 * \param txPowerLevel To expose the "TxPowerLevel" on the advertising message.
	 * \param connInterval To expose the "connection interval" on the advertising message.
	 * \param connTimeout  To expose the "connection timeout" on the advertising message.
	 * \param manufactureData To expose the "Manufacture Data" on the advertising message.
	 * \remark advertising(...) have to call before the enable() member.
	 * \note The connection interval must be between 8 ms and 4 s.
	 * \see bleSerial()
	 */
	void advertising(uint16_t advInterval,
			   int8_t txPowerLevel=DEF_BLE_TXPOWER,
			   uint16_t connInterval=DEF_BLE_CONN_INTERVAL,
			   uint16_t connTimeout=DEF_BLE_CONN_TIMEOUT,
			   uint32_t manufactureData=DEF_BLE_MFG_DATA);

	/**An inline function redirect to advertising() member function.
	 * \see advertising
	 */
	inline void setup(uint16_t advInterval,
			   int8_t txPowerLevel=DEF_BLE_TXPOWER,
			   uint16_t connInterval=DEF_BLE_CONN_INTERVAL,
			   uint16_t connTimeout=DEF_BLE_CONN_TIMEOUT,
			   uint32_t manufactureData=DEF_BLE_MFG_DATA) {
		advertising(advInterval, txPowerLevel, connInterval, connTimeout, manufactureData);
	}

	//
	// Controls
	//

	/**The enable member is to call the CThread:start() to start the ble engine task.
	 * \code
	 * #include "class/ble_serial.h"
	 * int main() {
	 * 		bleSerial ble("myBLE");
	 * 		ble.enable();	// to start the BLE core and Task.
	 * 		...
	 * 		...
	 * }
	 * \endcode
	 * \param stack To indicate the stack size of BLE task. default is 128 bytes.
	 * \return true if start the ble task successful, otherwise is failed.
	 */
#ifdef DEBUG
	bool enable(uint32_t stack=190);
#else
	bool enable(uint32_t stack=132);
#endif

	/**The disable member is to suspend the bleSerail task.
	 * \note Call enable member to resume the bleSerail task.
	 */
	void disable();

	/**Enable a watchdog on a BLE connection. The watchdog feature will cause the BLE core reset
	 * when remote (App) crash or lose the connection.
	 * \param tm A timeout value in millisecond, recommend value is 500~30000. If set the tm to zero, it is meant to disable the watchdog.
	 * \note The member is an optional function, and default is 10,000ms (10 seconds).
	 */
	void watchdog(uint32_t tm);		// The WD timeout will reset the BLE core automatically when APP crash to cause the BLE core lock,

	//
	// PHY Function
	//
	/**To disconnect current connection with a reason.
	 * \param reason is a BLE_DISCONNECT_REASON_T enumeration.
	 * \return true if disconnect successful, otherwise, disconnect failed.
	 */
	bool	disconnect(BLE_DISCONNECT_REASON_T reason=BLE_TERMINATED, bool internal=false);

	/**Set the ouptut power level of the Bluetooth Low Energy radio.
	 * \param power is a BLE_TX_POWER_T enumeration.
	 * \return true if set radio power successful, otherwise is failed.
	 */
	bool	setRadioTxPower(BLE_TX_POWER_T power);

	/**Get BLE core hardware version.
	 * \return An uint8_t type value.
	 */
	uint8_t getPhyVersion();

	/**Set BLE core into sleep mode
	 */
	void sleep();

	/**Weakup BLE core from sleep mode
	 */
	void wakeup();

	/**Block & waiting for a BLE event, the task will be blocked until a BLE event arrived.
	 */
	bool wait(uint32_t timeout=MAX_DELAY_TIME);

	/**TXD flow control
	 * \param Set true to enable the TXD flow control, false otherwise.
	 * \note The TXD flow control default is enabled.
	 * \warning Don't supported in V2.
	 */
//	 void txdFlowControl(bool value); // Mask in V2

	//
	// Events
	//

	/**An virtual function call by BLE task and occurs when remote (App) is already to connect the BLE device.
	 * \remark To override the virtual, the onConnection of child have to call the onConnection of supper class.
	 * \code
	 * class myBle: public bleSerial {
	 * public:
	 * 		// override the onConnected() virtual function
	 * 		virtual onConnected() {
	 * 			bleSerial::onConnection();		// call to parent class
	 *
	 * 			// your onConnection event code here:
	 * 			...
	 * 			...
	 * 		}
	 * 	};
	 * 	\endcode
	 */
	virtual void onConnected();

	/**An virtual function call by BLE task and occurs when remote (App) is already to disconnect the BLE device.
	 * \remark To override the virtual, the onDisconnection of child have to call the onDisconnection of parent class.
	 * \code
	 * class myBle: public bleSerial {
	 * public:
	 * 		// override the onConnected() virtual function
	 * 		virtual onDisconnected() {
	 * 			bleSerial::onDisconnection();	// call to parent class
	 *
	 * 			// your onDisonnection event code here:
	 * 			...
	 * 			...
	 * 		}
	 * 	};
	 * 	\endcode
	 */
	virtual void onDisconnected();

	/**An virtual function call by BLE task and occurs when a watchdog timeout on a connection.
	 * \remark The onWatchdog member will call the reset() member function to reset the BLE core.
	 */
	virtual void onWatchdog();

	/**An virtual function call by BLE task and occurs when a BLE hardware error.
	 * \param err is a BLE_ERR_T enumeration.
	 * \param id is a string to point a class name. (for debug)
	 * \note The onError event is a empty function in bleSerial class.
	 */
	virtual void onError(BLE_ERR_T err, LPCTSTR id="bleSerial"){}	// onError() Event indicate the error code when an error occurred.

	//
	// for BLE UART Service
	// Implement the virtual functions of CStream class
	//

	/**Use isAvailable to check the Serial service whether opened by remote (App).
	 * \return true, if service is available. otherwise, the service is not in used.
	 * \note This isAvailable member is an inline function to redirect to the writeable() member.
	 * \see writeable
	 */
	inline  bool isAvailable() { return writeable(); }

	//
	// Implement the virtual functions from CStream class
	//
	virtual int readable();
	virtual int writeable();
	virtual bool isConnected();

	//
	// PRIVATE, internal used
	//
	/*! \cond PRIVATE */
	virtual ~bleSerial();
			void reset();
			uint16_t status();

	virtual void onRecv(uint8_t *buf, int len);
	virtual void onSend(bool fromISR);
//	virtual void onACK(uint8_t data);

	virtual void onResponseCommandHook(void *data);
	virtual void onTransactionFinishedHook();
	virtual void onDataCreditHook();
//	virtual void onAckTimeout();		// Mask in V2
	virtual bool isReadyForNotify();

	virtual bool isActiveMode();
	virtual void inActiveMode(bool enable);

	virtual void onTestMode() {}		// Event for Enter Test Mode
	virtual void onStandbyMode() {}		// Event for Enter Standby Mode

	LPCTSTR	 m_deviceName;
	uint16_t m_advInterval;
	uint16_t m_conInterval;
	uint16_t m_conTimeout;
	uint32_t m_mfgData;
	int8_t	 m_txPowerLevel;
#if SUPPORT_BLE_PROXIMITY
	virtual void onAlert(uint8_t level);
	virtual void onLinkLoss(uint8_t level);
#endif
	/**onHeartRateControl is call by BLE task.
	 * \param ctrl if 1, resets the value of the Energy Expended field in the Heart Rate Measurement. otherwise reserved.
	 */
	virtual void onHeartRateControl(uint8_t ctrl);
protected:
	//
	// Implement the virtual function of CThread class
	//
	virtual void run();

protected:
	void releaseAll();
	bool waitForResponse();
	CMutex			m_mxSender;
	CSemaphore		m_semResponse;
	CSemaphore		m_semTransaction;
	CSemaphore		m_semDataCredit;
	CSemaphore		m_semWait;
	xHandle			m_event;

private:
//	CTimeout	m_tmACK;	// Mask in V2
	uint32_t	m_tmWatch;

	//
	// Friend Classes
	//
#include "class/ble_battery.h"
#include "class/ble_ht.h"
#if SUPPORT_BLE_PROXIMITY
#include "class/ble_proximity.h"	// Add @ V2
#endif
#include "class/ble_rsc.h"
#include "class/ble_csc.h"
#include "class/ble_heartrate.h"
#include "class/ble_devinfo.h"
#include "class/ble_test.h"
friend class bleBatteryLevel;
friend class bleHealthThermometer;
friend class bleProximity;		// Add @ V2
friend class bleRSC;				// Add @ uCXpresso.BLE v1.0.7
friend class bleCSC;
friend class bleHeartRate;
friend class bleDeviceInfo;
friend class bleTest;
#if SUPPORT_BLE_PROXIMITY
	class bleProximity	*m_proximity;
#endif
	class bleHeartRate	*m_heartrate;

#undef BLE_BATTERY_H_
#undef BLE_HT_H_
#undef BLE_PROXIMITY_H_
#undef BLE_HEARTRATE_H_
#undef BLE_DEVINFO_H_
#undef BLE_TEST_H_
#undef BLE_RSC_H_
	/*! \endcond */
};

/**\example /ble/ble_multiple/src/main.cpp
 * This is an example of how to use the bleSerial class.
 * More details about this example.
 */

/// \cond
extern bleSerial *objBLE;
/// \endcond

#endif /* BLE_SERIAL_H */
