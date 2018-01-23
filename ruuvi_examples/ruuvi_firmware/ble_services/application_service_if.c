#include "application_service_if.h"
#include <stdint.h>

#include "app_scheduler.h"
#include "nrf_delay.h"

#include "ble_nus.h"
#include "ble_dfu.h"

#include "ble_bulk_transfer.h"
#include "ruuvi_endpoints.h"

#define NRF_LOG_MODULE_NAME "SERVICE"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

static ble_nus_t m_nus;     /**< Structure to identify the Nordic UART Service. */
static ble_dfu_t m_dfus;    /**< Structure for DFU service. */

static void nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
  NRF_LOG_INFO("Received %s\r\n", (uint32_t)p_data);
  //Assume standard message - TODO: Switch by endpoint
  if(length == 11){
    ruuvi_standard_message_t message = { .destination_endpoint = p_data[0],
                                         .source_endpoint = p_data[1],
                                         .type = p_data[2],
                                         .payload = {0}};
    memcpy(&(message.payload[0]), &(p_data[3]), sizeof(message.payload));
    //Schedule handling of the message - do not process in interrupt context
    app_sched_event_put	(	&message,
                          sizeof(message),
                          ble_gatt_scheduler_event_handler);
  }
}

static void ble_dfu_evt_handler(ble_dfu_t * p_dfu, ble_dfu_evt_t * p_evt)
{
    switch (p_evt->type)
    {
        case BLE_DFU_EVT_INDICATION_DISABLED:
            NRF_LOG_INFO("Indication for BLE_DFU is disabled\r\n");
            break;

        case BLE_DFU_EVT_INDICATION_ENABLED:
            NRF_LOG_INFO("Indication for BLE_DFU is enabled\r\n");
            break;

        case BLE_DFU_EVT_ENTERING_BOOTLOADER:
            NRF_LOG_INFO("Device is entering bootloader mode!\r\n");
            break;
        default:
            NRF_LOG_INFO("Unknown event from ble_dfu\r\n");
            break;
    }
}

/**@brief Function for initializing the Services generated by Bluetooth Developer Studio.
 *
 *
 * @return      NRF_SUCCESS on successful initialization of services, otherwise an error code.
 */
uint32_t application_services_init(void)
{
    uint32_t       err_code = NRF_SUCCESS;

    //NUS
    ble_nus_init_t nus_init;

    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code |= ble_nus_init(&m_nus, &nus_init);

    NRF_LOG_INFO("NUS Init status: %s\r\n", (uint32_t)ERR_TO_STR(err_code));
    nrf_delay_ms(10);
    
    // Initialize the Device Firmware Update Service.
    ble_dfu_init_t dfus_init;
    memset(&dfus_init, 0, sizeof(dfus_init));

    dfus_init.evt_handler                               = ble_dfu_evt_handler;
    dfus_init.ctrl_point_security_req_write_perm        = SEC_SIGNED;
    dfus_init.ctrl_point_security_req_cccd_write_perm   = SEC_SIGNED;

    err_code |= ble_dfu_init(&m_dfus, &dfus_init);
    NRF_LOG_INFO("DFU Init status: %s\r\n", (uint32_t)ERR_TO_STR(err_code));
    nrf_delay_ms(10);
  
    return err_code;
}

/** Return pointer to BLE nus service **/
ble_nus_t* get_nus(void)
{
  return &m_nus;
}

/** Return pointer to BLE dfu service **/
ble_dfu_t* get_dfu(void)
{
  return &m_dfus;
}