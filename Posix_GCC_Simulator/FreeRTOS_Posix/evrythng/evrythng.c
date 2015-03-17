#include <string.h>
#include <FreeRTOS.h>
#include "MQTTClient.h"
#include "MQTTClientPersistence.h"
#include "evrythng.h"

#define malloc pvPortMalloc
#define free vPortFree

//extern int wmprintf(const char *format, ...);
#define wmprintf printf
//#define dbg(_fmt_, ...) wmprintf(_fmt_"\n\r", ##__VA_ARGS__)
//#define log(_fmt_, ...) wmprintf(_fmt_"\n\r", ##__VA_ARGS__)
#define err(_fmt_, ...) wmprintf("Error: "_fmt_"\n\r", ##__VA_ARGS__)
#define dbg(...)
#define log(...)
//#define err(...)

#if defined (OPENSSL) || defined (TLSSOCKET)
MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
static const char *uristring;
#endif

#define JSON_NUM_TOKENS	10

#define TOPIC_MAX_LEN 128

evrythng_return_e EvrythngConnect(void);

MQTTClient evrythng_client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
int evrythng_initialized = 0;

typedef struct t_sub_callback {
  char*                   topic;
  int                     qos;
  sub_callback*           callback;
  struct t_sub_callback*  next;
} t_sub_callback;

t_sub_callback *t_sub_callbacks=NULL;

typedef struct t_pub_callback {
  MQTTClient_deliveryToken  dt;
  pub_callback*             callback;
  struct t_pub_callback*    next;
} t_pub_callback;

t_pub_callback *t_pub_callbacks=NULL;

static evrythng_return_e add_sub_callback(char* topic, int qos, sub_callback *callback)
{
  t_sub_callback **_t_sub_callbacks=&t_sub_callbacks;
  while(*_t_sub_callbacks) {
	_t_sub_callbacks = &(*_t_sub_callbacks)->next;
  }
  if ((*_t_sub_callbacks = (t_sub_callback*)malloc(sizeof(t_sub_callback))) == NULL) {
	return EVRYTHNG_FAILURE;
  }
  if (((*_t_sub_callbacks)->topic = (char*)malloc(strlen(topic) + 1)) == NULL) {
	free(*_t_sub_callbacks);
	return EVRYTHNG_FAILURE;
  }
  strcpy((*_t_sub_callbacks)->topic, topic);
  (*_t_sub_callbacks)->qos = qos;
  (*_t_sub_callbacks)->callback = callback;
  (*_t_sub_callbacks)->next = NULL;

  return EVRYTHNG_SUCCESS;
}

static evrythng_return_e add_pub_callback(MQTTClient_deliveryToken dt, pub_callback *callback)
{
  t_pub_callback **_t_pub_callbacks=&t_pub_callbacks;
  while(*_t_pub_callbacks) {
	if ((*_t_pub_callbacks)->callback == NULL) {
	  (*_t_pub_callbacks)->dt = dt;
	  (*_t_pub_callbacks)->callback = callback;
	  return EVRYTHNG_SUCCESS;
	}
	_t_pub_callbacks = &(*_t_pub_callbacks)->next;
  }
  if ((*_t_pub_callbacks = (t_pub_callback*)malloc(sizeof(t_pub_callback))) == NULL) {
	return EVRYTHNG_FAILURE;
  }
  (*_t_pub_callbacks)->dt = dt;
  (*_t_pub_callbacks)->callback = callback;
  (*_t_pub_callbacks)->next = NULL;

  return EVRYTHNG_SUCCESS;
}

void connectionLost_callback(void* context, char* cause)
{
  err("Callback: connection lost");

  t_pub_callback **_t_pub_callbacks=&t_pub_callbacks;
  while(*_t_pub_callbacks) {
	(*_t_pub_callbacks)->callback = NULL;
	_t_pub_callbacks = &(*_t_pub_callbacks)->next;
  }

  while (EvrythngConnect() != EVRYTHNG_SUCCESS)
  {
	vTaskDelay(1000/portTICK_RATE_MS);
  }
}

int message_callback(void* context, char* topic_name, int topic_len, MQTTClient_message* message)
{
  log("topic: %s", topic_name);
  log("Received: %s", (char*)message->payload);

  if (message->payloadlen < 3) {
	err("Wrong message lenght");
	goto exit;
  }

  t_sub_callback **_t_sub_callbacks=&t_sub_callbacks;
  while(*_t_sub_callbacks) {
	if (strcmp((*_t_sub_callbacks)->topic, topic_name) == 0) {
	  (*((*_t_sub_callbacks)->callback))((char*)message->payload);
	}
	_t_sub_callbacks = &(*_t_sub_callbacks)->next;
  }

exit:
  MQTTClient_freeMessage(&message);
  MQTTClient_free(topic_name);

  return 1;
}

void deliveryComplete_callback(void* context, MQTTClient_deliveryToken dt)
{
  t_pub_callback **_t_pub_callbacks=&t_pub_callbacks;
  while(*_t_pub_callbacks) {
	if ((*_t_pub_callbacks)->dt == dt) {
	  (*((*_t_pub_callbacks)->callback))();
	  (*_t_pub_callbacks)->callback = NULL;
	  return;
	}
	_t_pub_callbacks = &(*_t_pub_callbacks)->next;
  }
}

evrythng_return_e EvrythngConfigure(evrythng_config_t* config)
{
  if (evrythng_initialized) {
	return EvrythngConnect();
  }

  conn_opts.keepAliveInterval = 10;
  conn_opts.reliable = 0;
  conn_opts.cleansession = 1;
  conn_opts.username = (char*)pvPortMalloc(strlen("authorization")*sizeof(char)+1);
  strcpy(conn_opts.username, "authorization");
  conn_opts.password = (char*)pvPortMalloc(strlen(config->api_key)*sizeof(char)+1);
  strcpy(conn_opts.password, config->api_key);
  conn_opts.struct_version = 1;
#if defined (OPENSSL) || defined (TLSSOCKET)
  if (config->enable_ssl)
  {
	ssl_opts.enableServerCertAuth = 0;
	ssl_opts.trustStore = (char*)pvPortMalloc(config->cert_buffer_size+1); 
	strcpy(ssl_opts.trustStore, config->cert_buffer);
	//ssl_opts.trustStore_size = config->cert_buffer_size;

	conn_opts.serverURIcount = 1;
	uristring = (char*)pvPortMalloc(strlen(config->tls_server_uri)*sizeof(char)+1);
	strcpy(uristring, config->tls_server_uri);
	conn_opts.serverURIs = &uristring;
	conn_opts.struct_version = 4;
	conn_opts.ssl = &ssl_opts;
  }
#endif

  MQTTClient_init();

  if(MQTTClient_create(&evrythng_client, config->url, config->client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS)
  {
	err("Can't create client");
	return EVRYTHNG_FAILURE;
  }

  if(MQTTClient_setCallbacks(evrythng_client, evrythng_client, connectionLost_callback, message_callback, deliveryComplete_callback) != MQTTCLIENT_SUCCESS)
  {
	err("Can't set callback");
	return EVRYTHNG_FAILURE;
  }

  evrythng_initialized = 1;

  return EvrythngConnect();
}

evrythng_return_e EvrythngConnect(void)
{
  if (MQTTClient_isConnected(evrythng_client)) {
	return EVRYTHNG_SUCCESS;
  }

  int rc;

  log("MQTT Connecting");
  if ((rc = MQTTClient_connect(evrythng_client, &conn_opts)) != MQTTCLIENT_SUCCESS)
  {
	err("Failed to connect, return code %d", rc);
	return EVRYTHNG_FAILURE;
  }
  log("MQTT Connected");

  t_sub_callback **_t_sub_callbacks=&t_sub_callbacks;
  while(*_t_sub_callbacks) {
	int rc = MQTTClient_subscribe(evrythng_client, (*_t_sub_callbacks)->topic, (*_t_sub_callbacks)->qos);
	if (rc == MQTTCLIENT_SUCCESS) {
	  log("Subscribed: %s", (*_t_sub_callbacks)->topic);
	}
	else {
	  err("rc=%d", rc);
	}
	_t_sub_callbacks = &(*_t_sub_callbacks)->next;
  }

  return EVRYTHNG_SUCCESS;
}

static evrythng_return_e EvrythngSub(char* entity, char* entity_id, char* data_type, char* data_name, int qos, sub_callback *callback)
{
  if (!MQTTClient_isConnected(evrythng_client)) {
	err("Client doesn't started");
	return EVRYTHNG_FAILURE;
  }

  int rc;
  char sub_topic[TOPIC_MAX_LEN];

  if (entity_id == NULL) {
	rc = snprintf(sub_topic, TOPIC_MAX_LEN, "%s/%s", entity, data_name);
	if (rc < 0 || rc >= TOPIC_MAX_LEN) {
	  err("Topic overflow");
	  return EVRYTHNG_FAILURE;
	}
  } else if (data_name == NULL) {
	rc = snprintf(sub_topic, TOPIC_MAX_LEN, "%s/%s/%s", entity, entity_id, data_type);
	if (rc < 0 || rc >= TOPIC_MAX_LEN) {
	  err("Topic overflow");
	  return EVRYTHNG_FAILURE;
	}
  } else {
	rc = snprintf(sub_topic, TOPIC_MAX_LEN, "%s/%s/%s/%s", entity, entity_id, data_type, data_name);
	if (rc < 0 || rc >= TOPIC_MAX_LEN) {
	  err("Topic overflow");
	  return EVRYTHNG_FAILURE;
	}
  }

  rc = add_sub_callback(sub_topic, qos, callback);
  if (rc != EVRYTHNG_SUCCESS) {
	err("Can't allocate memory");
	return EVRYTHNG_FAILURE;
  }

  log("s_t: %s", sub_topic);

  rc = MQTTClient_subscribe(evrythng_client, sub_topic, qos);
  if (rc == MQTTCLIENT_SUCCESS) {
	log("Subscribed: %s", sub_topic);
  }
  else {
	err("rc=%d", rc);
	return EVRYTHNG_FAILURE;
  }

  return EVRYTHNG_SUCCESS;
}

static evrythng_return_e EvrythngPub(char* entity, char* entity_id, char* data_type, char* data_name, char* property_json, int qos, pub_callback *callback)
{
  if (!MQTTClient_isConnected(evrythng_client)) {
	err("Client doesn't started");
	return EVRYTHNG_FAILURE;
  }

  int rc;
  char pub_topic[TOPIC_MAX_LEN];

  if (entity_id == NULL) {
	rc = snprintf(pub_topic, TOPIC_MAX_LEN, "%s/%s", entity, data_name);
	if (rc < 0 || rc >= TOPIC_MAX_LEN) {
	  err("Topic overflow");
	  return EVRYTHNG_FAILURE;
	}
  } else if (data_name == NULL) {
	rc = snprintf(pub_topic, TOPIC_MAX_LEN, "%s/%s/%s", entity, entity_id, data_type);
	if (rc < 0 || rc >= TOPIC_MAX_LEN) {
	  err("Topic overflow");
	  return EVRYTHNG_FAILURE;
	}
  } else {
	rc = snprintf(pub_topic, TOPIC_MAX_LEN, "%s/%s/%s/%s", entity, entity_id, data_type, data_name);
	if (rc < 0 || rc >= TOPIC_MAX_LEN) {
	  err("Topic overflow");
	  return EVRYTHNG_FAILURE;
	}
  }

  log("p_t: %s", pub_topic);

  MQTTClient_deliveryToken dt;
  rc = MQTTClient_publish(evrythng_client, pub_topic, strlen(property_json), property_json, qos, 0, &dt);
  if (rc == MQTTCLIENT_SUCCESS) {
	log("Published: %s", property_json);
  }
  else {
	err("rc=%d", rc);
	return EVRYTHNG_FAILURE;
  }

  if (callback != NULL && qos != 0) {
	if (add_pub_callback(dt, callback) != EVRYTHNG_SUCCESS) {
	  err("Can't add callback");
	  return EVRYTHNG_FAILURE;
	}
  }

  return EVRYTHNG_SUCCESS;
}

evrythng_return_e EvrythngSubThngProperty(char* thng_id, char* property_name, int qos, sub_callback *callback)
{
  return EvrythngSub("thngs", thng_id, "properties", property_name, qos, callback);
}

evrythng_return_e EvrythngSubThngProperties(char* thng_id, int qos, sub_callback *callback)
{
  return EvrythngSub("thngs", thng_id, "properties", NULL, qos, callback);
}

evrythng_return_e EvrythngPubThngProperty(char* thng_id, char* property_name, char* property_json, int qos, pub_callback *callback)
{
  return EvrythngPub("thngs", thng_id, "properties", property_name, property_json, qos, callback);
}

evrythng_return_e EvrythngPubThngProperties(char* thng_id, char* properties_json, int qos, pub_callback *callback)
{
  return EvrythngPub("thngs", thng_id, "properties", NULL, properties_json, qos, callback);
}

evrythng_return_e EvrythngSubThngAction(char* thng_id, char* action_name, int qos, sub_callback *callback)
{
  return EvrythngSub("thngs", thng_id, "actions", action_name, qos, callback);
}

evrythng_return_e EvrythngSubThngActions(char* thng_id, int qos, sub_callback *callback)
{
  return EvrythngSub("thngs", thng_id, "actions", "all", qos, callback);
}

evrythng_return_e EvrythngPubThngAction(char* thng_id, char* action_name, char* action_json, int qos, pub_callback *callback)
{
  return EvrythngPub("thngs", thng_id, "actions", action_name, action_json, qos, callback);
}

evrythng_return_e EvrythngPubThngActions(char* thng_id, char* actions_json, int qos, pub_callback *callback)
{
  return EvrythngPub("thngs", thng_id, "actions", "all", actions_json, qos, callback);
}

evrythng_return_e EvrythngSubThngLocation(char* thng_id, int qos, sub_callback *callback)
{
  return EvrythngSub("thngs", thng_id, "location", NULL, qos, callback);
}

evrythng_return_e EvrythngPubThngLocation(char* thng_id, char* location_json, int qos, pub_callback *callback)
{
  return EvrythngPub("thngs", thng_id, "location", NULL, location_json, qos, callback);
}


evrythng_return_e EvrythngSubProductProperty(char* products_id, char* property_name, int qos, sub_callback *callback)
{
  return EvrythngSub("products", products_id, "properties", property_name, qos, callback);
}

evrythng_return_e EvrythngSubProductProperties(char* products_id, int qos, sub_callback *callback)
{
  return EvrythngSub("products", products_id, "properties", NULL, qos, callback);
}

evrythng_return_e EvrythngPubProductProperty(char* products_id, char* property_name, char* property_json, int qos, pub_callback *callback)
{
  return EvrythngPub("products", products_id, "properties", property_name, property_json, qos, callback);
}

evrythng_return_e EvrythngPubProductProperties(char* products_id, char* properties_json, int qos, pub_callback *callback)
{
  return EvrythngPub("products", products_id, "properties", NULL, properties_json, qos, callback);
}

evrythng_return_e EvrythngSubProductAction(char* products_id, char* action_name, int qos, sub_callback *callback)
{
  return EvrythngSub("products", products_id, "actions", action_name, qos, callback);
}

evrythng_return_e EvrythngSubProductActions(char* products_id, int qos, sub_callback *callback)
{
  return EvrythngSub("products", products_id, "actions", "all", qos, callback);
}

evrythng_return_e EvrythngPubProductAction(char* products_id, char* action_name, char* action_json, int qos, pub_callback *callback)
{
  return EvrythngPub("products", products_id, "actions", action_name, action_json, qos, callback);
}

evrythng_return_e EvrythngPubProductActions(char* products_id, char* actions_json, int qos, pub_callback *callback)
{
  return EvrythngPub("products", products_id, "actions", "all", actions_json, qos, callback);
}


evrythng_return_e EvrythngSubAction(char* action_name, int qos, sub_callback *callback)
{
  return EvrythngSub("actions", NULL, NULL, action_name, qos, callback);
}

evrythng_return_e EvrythngSubActions(int qos, sub_callback *callback)
{
  return EvrythngSub("actions", NULL, NULL, "all", qos, callback);
}

evrythng_return_e EvrythngPubAction(char* action_name, char* action_json, int qos, pub_callback *callback)
{
  return EvrythngPub("actions", NULL, NULL, action_name, action_json, qos, callback);
}

evrythng_return_e EvrythngPubActions(char* actions_json, int qos, pub_callback *callback)
{
  return EvrythngPub("actions", NULL, NULL, "all", actions_json, qos, callback);
}

