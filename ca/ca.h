#ifndef EBPC_CA_H
#define EBPC_CA_H

#include <iostream>
#include <thread>

#include "../proto/ca_protomsg.pb.h"
#include "../net/msg_queue.h"

/**
* @description: ca initialization 
* @param no 
* @return: Return true on success, false on failure 
*/
bool ca_init();


/**
 * @description: ca cleanup function 
 * @param no 
 * @return: no 
 */
void ca_cleanup();

/**
 * @description: ca menu 
 * @param no 
 * @return: no 
 */
void ca_print();


/**
 * @description: ca version 
 * @param no 
 * @return: Returns the version number in string format 
 */
const char * ca_version();


/**
 * @description: Get the height and hash of the highest block of a node 
 * @param id The id of the node to get 
 * @return: no 
 */
void SendDevInfoReq(const std::string id);


/* ====================================================================================  
 # @description:  Process the received node information 
 # @param msg  : Protocol data received 
 # @param msgdata : Data required for network transmission 
 ==================================================================================== */
void HandleGetDevInfoAck( const std::shared_ptr<GetDevInfoAck>& msg, const MsgData& msgdata );


/* ====================================================================================  
 # @description: Process the received node information 
 # @param msg  : Protocol data received 
 # @param msgdata : Data required for network transmission 
 ==================================================================================== */
void HandleGetDevInfoReq( const std::shared_ptr<GetDevInfoReq>& msg, const MsgData& msgdata );


/**
 * @description: Related implementation functions used in the main menu 
 * @create: 20201104   LiuMingLiang
 */
void ca_print_basic_info();
int set_device_signature_fee(uint64_t fee);
int set_device_package_fee(uint64_t fee);
int get_device_package_fee(uint64_t& packageFee);

void handle_transaction();
void handle_pledge();
void handle_redeem_pledge();
bool isPublicIp(const string& ip);
int UpdatePublicNodeToConfigFile();
void AutoTranscation();

unsigned int get_chain_height();

#endif
