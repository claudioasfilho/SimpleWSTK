/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include "em_common.h"
#include "sl_app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "sl_app_log.h"

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}



#define ARRAYSIZE 100

#define TIMINGTOUPDATE 246

static uint8_t customCharData[ARRAYSIZE];
static uint8_t ConnectionHandle;
static uint16_t NotificationsEnabled =0;


void updateCustomData(uint8_t data)
{

  int i;
  for (i=0; i<ARRAYSIZE; i++)
    {
      customCharData[i] = i;
    }

  sl_bt_gatt_server_write_attribute_value(gattdb_customChar,
                                          0,
                                           sizeof(customCharData),
                                           &customCharData
                                           );

  sl_app_log((char)customCharData);
  sl_app_log("\r\n");
}

void fillCustomData()
{


  for (int i=0; i<ARRAYSIZE; i++)
    {
      customCharData[i] = i;
    }

  sl_bt_gatt_server_write_attribute_value(gattdb_customChar,
                                          0,
                                           sizeof(customCharData),
                                           &customCharData
                                           );

  printf("Data: %s \r\n",customCharData);
}


static uint32_t NotiCounter = 0;
//static uint8_t NotiCounter = 0;

void sendNotification()
{


customCharData[99] = (NotiCounter & 0xff000000UL) >> 24;
customCharData[98] = (NotiCounter & 0x00ff0000UL) >> 16;
customCharData[97] = (NotiCounter & 0x0000ff00UL) >>  8;
customCharData[96] = (NotiCounter & 0x000000ffUL)      ;
//  customCharData[99] = (NotiCounter & 0x000000ffUL);

  if (NotificationsEnabled == gatt_notification)
           {
            sl_bt_gatt_server_send_characteristic_notification(
                              ConnectionHandle,
                              gattdb_customChar,
                              sizeof(customCharData),
                              &customCharData,
                              sizeof(customCharData));
                          sl_app_log("Notification Sent %d \r\n", NotiCounter);
                          sl_app_log(customCharData);
                          sl_app_log("\r\n");
                          NotiCounter++;
           }
  //else printf("Notifications disabled\r\n");


}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
  ///

  sendNotification();

}
/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      sl_app_log("Device has booted up\r\n");
      sl_app_log("Simple App 100 Bytes\r\n");

      //updateCustomData(0);
      fillCustomData();
      //sl_bt_system_set_soft_timer ( TIMINGTOUPDATE, 254, 0);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to create advertising set\n",
                    (int)sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to set advertising timing\n",
                    (int)sc);
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:

      if (evt->data.evt_gatt_server_attribute_value.attribute == gattdb_customChar)
        {
          customCharData[0] = evt->data.evt_gatt_server_attribute_value.value.data[0];

          sl_app_log("customChar Characteristic %x \r\n", customCharData);

        }
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
          if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_customChar)
            {
              NotificationsEnabled =evt->data.evt_gatt_server_characteristic_status.client_config_flags;
            // client characteristic configuration changed by remote GATT client
            if ((gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags == 1)
              {
                //Starting a Soft-Timer to send the data back to Central
                sl_app_log("Central Subscribed to Characteristic\r\n");

                //sl_bt_system_set_soft_timer ( TIMINGTOUPDATE, 0, 0);
                sl_bt_gatt_server_read_attribute_value  ( gattdb_customChar,
                                                                       0,
                                                                        1,
                                                                        sizeof(customCharData),
                                                                        &customCharData
                                                                        );


              }
            // confirmation of indication received from remove GATT client
            else if (gatt_server_confirmation == (gatt_server_characteristic_status_flag_t)evt->data.evt_gatt_server_characteristic_status.status_flags)
              {
                sl_app_log("Gatt_server_confirmation\r\n");
              }
            else {
              sl_app_assert(false,
                            "[E: 0x%04x] Unexpected status flag in evt_gatt_server_characteristic_status\n",
                            (int)evt->data.evt_gatt_server_characteristic_status.status_flags);
            }
          }
          break;

    case  sl_bt_evt_system_soft_timer_id:

          if (evt->data.evt_system_soft_timer.handle == 0)
            {
              sl_bt_gatt_server_send_characteristic_notification(
                  ConnectionHandle,
                  gattdb_customChar,
                  sizeof(customCharData),
                  &customCharData,
                  sizeof(customCharData));
              sl_app_log("soft timer %x \r\n", customCharData);

            }

          if (evt->data.evt_system_soft_timer.handle == 254)
            {
             // static uint8_t counter = 0;

              //counter++;
              //updateCustomData(counter);
            }
      break;




    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      ConnectionHandle = evt->data.evt_connection_opened.connection;

      sl_bt_connection_set_parameters(ConnectionHandle,6,6,0,10,0,65535);

      sl_bt_connection_set_preferred_phy(ConnectionHandle, 2,0xff);


      break;

      //Triggered whenever the connection parameters are changed and at any time a connection is established.
    case sl_bt_evt_connection_parameters_id:

      sl_bt_connection_set_preferred_phy(ConnectionHandle, 2,0xff);

      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:

      NotificationsEnabled = gatt_disable;
      NotiCounter = 0;

      sl_app_log("Connection closed, soft timer disabled \r\n");
      sl_bt_system_set_soft_timer ( 0, 0, 0);


      // Restart advertising after client has disconnected.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_general_discoverable,
        advertiser_connectable_scannable);
      sl_app_assert(sc == SL_STATUS_OK,
                    "[E: 0x%04x] Failed to start advertising\n",
                    (int)sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
