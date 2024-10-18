#include "cryptoapp.h"
LOG_MODULE_REGISTER(cryptoapp,LOG_LEVEL_DBG);

#include "psa/crypto.h"
#include "psa/crypto_extra.h"



static psa_key_id_t key_id;


/* AES IV buffer */
//static uint8_t m_iv[16];



#define PRINT_HEX(p_label, p_text, len)\
	({\
		LOG_INF("---- %s (len: %u): ----", p_label, len);\
		LOG_HEXDUMP_INF(p_text, len, "Content:");\
		LOG_INF("---- %s end  ----", p_label);\
	})



GLOBAL enum cryptoapp_status cryptoapp_run(enum cryptoapp_func sel_func,struct cryptoapp_packet *p_packet)
{
     enum cryptoapp_status stat = cryptoapp_ok;


     switch(sel_func)
     {

        case cryptoapp_impKeyndEncrypt: //get the key and generate encryption onn that
        {
            stat = cryptoapp_init();
            stat = cryptoapp_importKey(p_packet->key,p_packet->key_size);
            stat = cryptoapp_encryptMsg(p_packet->msgBuf,p_packet->msgBuf_size,p_packet->encryptedMsgBuf
                                        ,p_packet->encryptedMsgBuf_size,p_packet->iv_buffer,p_packet->iv_size);
            stat = cryptoapp_finish();

            break;
        }

        case cryptoapp_impKeyndDecrypt: //get the key and generate decryption onn that
        {
            stat = cryptoapp_init();
            stat = cryptoapp_importKey(p_packet->key,p_packet->key_size);
            stat = cryptoapp_decryptMsg(p_packet->encryptedMsgBuf,p_packet->encryptedMsgBuf_size,p_packet->decryptedMsgBuf
                                         ,p_packet->decryptedMsgBuf_size,p_packet->iv_buffer,p_packet->iv_size);
            stat = cryptoapp_finish();
            break;
        }

        case cryptoapp_genKeyndEncrypt://generate the key and generate encryption onn that
        {
            stat = cryptoapp_init();
            stat = cryptoapp_generateKey();
            stat = cryptoapp_encryptMsg(p_packet->msgBuf,p_packet->msgBuf_size,p_packet->encryptedMsgBuf
                                        ,p_packet->encryptedMsgBuf_size,p_packet->iv_buffer,p_packet->iv_size);
            stat = cryptoapp_finish();
            break;
        }

        case cryptoapp_genKeyndDecrypt://get the key and generate decryption onn that
        {
          stat = cryptoapp_init();
          stat = cryptoapp_generateKey();
          stat = cryptoapp_decryptMsg(p_packet->encryptedMsgBuf,p_packet->encryptedMsgBuf_size,p_packet->decryptedMsgBuf
                                         ,p_packet->decryptedMsgBuf_size,p_packet->iv_buffer,p_packet->iv_size);
          stat = cryptoapp_finish();

            break;
        }

     }


//common:
     return stat;

}    

/*
Initialies cryptographic engine
 */
GLOBAL enum cryptoapp_status cryptoapp_init()
{
    enum cryptoapp_status stat = cryptoapp_ok;
    psa_status_t status;

    status = psa_crypto_init();
    if(status != PSA_SUCCESS)
    {
        LOG_ERR("Failed to Initialize"); 
        stat = cryptoapp_err;
    }
    
    
    LOG_INF("Initialization Successfull"); 

    return stat;

}

/*
Destroys cryptographic engine
*/
GLOBAL enum cryptoapp_status cryptoapp_finish()
{
      enum cryptoapp_status stat = cryptoapp_ok;
      psa_status_t status;

      	/* Destroy the key handle */
	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_destroy_key failed! (Error: %d)", status);
		stat = cryptoapp_err;
	}


 LOG_INF("Key destroyed Successfully"); 


    return stat;

}

/*
Generates Custom Keys
*/
GLOBAL enum cryptoapp_status cryptoapp_generateKey()
{
       enum cryptoapp_status stat = cryptoapp_ok;
       psa_status_t status;

       LOG_INF("Generating AES Key...");

       	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes,
				PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CTR);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 128);

    	status = psa_generate_key(&key_attributes, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_generate_key failed! (Error: %d)", status);
		stat = cryptoapp_err;
        goto common;
	}


	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	LOG_INF("AES key generated successfully!");


common:
     return stat;

}


/*
Use already generated Key

input 1: Pass the key
input 2: size of key
*/
GLOBAL enum cryptoapp_status cryptoapp_importKey(uint8_t *key,size_t key_size)
{
   enum cryptoapp_status stat = cryptoapp_ok;
   psa_status_t status;


     	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

    psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_lifetime(&key_attributes,  PSA_KEY_LIFETIME_VOLATILE);
    psa_set_key_algorithm(&key_attributes, PSA_ALG_CTR);
    psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&key_attributes, 128);

    // Import the AES key
    status = psa_import_key(&key_attributes, key, key_size, &key_id);
    if (status != PSA_SUCCESS) {
        LOG_ERR("psa_import_key failed: %d\n", status);
       	stat = cryptoapp_err;
        goto common;
    }




common:
    return stat;
}


/*
Encrypt the given message

input 1: message which need to be encrypted
input 2: sizeof that message
input 3: buffer to store after the message has been encrypted
input 4: pass the sizeof encrypted message buffer
input 5: pass the buffer to store counter value for aes
input 6: pass the sizeof counter buffer
*/
GLOBAL enum cryptoapp_status cryptoapp_encryptMsg(void *encrypt_msg , size_t encrypt_size , void *encrypted_msg , size_t encrypted_size,uint8_t *iv_buffer,size_t iv_size)
{
   enum cryptoapp_status stat = cryptoapp_ok;
   psa_status_t status;
   uint32_t olen;
   	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	LOG_INF("Encrypting using AES CTR MODE...");
	/* Setup the encryption operation */
	status = psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_CTR);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_encrypt_setup failed! (Error: %d)", status);
		 	stat = cryptoapp_err;
        goto common;
	}


    	/* Generate a random IV */
	status = psa_cipher_generate_iv(&operation, iv_buffer, iv_size,
					&olen);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_generate_iv failed! (Error: %d)", status);
		stat = cryptoapp_err;
        goto common;
	}

	/* Perform the encryption */
	status = psa_cipher_update(&operation,
							   (const uint8_t *)encrypt_msg,
							   encrypt_size,
							   (uint8_t *)encrypted_msg,
							   encrypted_size,
							   &olen);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_update failed! (Error: %d)", status);
			stat = cryptoapp_err;
        goto common;
	}

    	/* Finalize encryption */
	status = psa_cipher_finish(&operation,
							   (uint8_t *)(encrypted_msg) + olen,
							   encrypted_size - olen,
							   &olen);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_finish failed! (Error: %d)", status);
			stat = cryptoapp_err;
        goto common;
	}


    /* Clean up cipher operation context */
	status = psa_cipher_abort(&operation);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_abort failed! (Error: %d)", status);
			stat = cryptoapp_err;
        goto common;
	}


    LOG_INF("Encryption successful!\r\n");
	PRINT_HEX("IV", iv_buffer, iv_size);
	PRINT_HEX("Plaintext", (uint8_t *)encrypt_msg, encrypt_size);
	PRINT_HEX("Encrypted text", (uint8_t *)encrypted_msg, encrypted_size);


common:
    return stat;
}

/*
Decrypt the given message

input 1: pass the message that has been encrypted
input 2: sizeof encrypted message
input 3: buffer to store the decrypted message
input 4: pass the sizeof decrypt message buffer
input 5: pass the counter value for aes
input 6: pass the sizeof that counter
*/
GLOBAL enum cryptoapp_status cryptoapp_decryptMsg(void *encrypted_msg , size_t encrypted_size, void *decrypt_msg , size_t decrypt_size,uint8_t *iv_buffer,size_t iv_size)
{
    uint32_t olen;
   enum cryptoapp_status stat = cryptoapp_ok;
psa_status_t status;

	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	LOG_INF("Decrypting using AES CTR MODE...");

    /* Setup the decryption operation */
	status = psa_cipher_decrypt_setup(&operation, key_id, PSA_ALG_CTR);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_decrypt_setup failed! (Error: %d)", status);
			stat = cryptoapp_err;
        goto common;
	}

    /* Set the IV to the one generated during encryption */
	status = psa_cipher_set_iv(&operation, iv_buffer, iv_size);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_set_iv failed! (Error: %d)", status);
			stat = cryptoapp_err;
        goto common;
	}


    /* Perform the decryption */
	status = psa_cipher_update(&operation,
							   (const uint8_t *)encrypted_msg,
							   encrypted_size,
							   (uint8_t *)decrypt_msg,
							   decrypt_size, &olen);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_update failed! (Error: %d)", status);
			stat = cryptoapp_err;
        goto common;
	}


    	/* Finalize the decryption */
	status = psa_cipher_finish(&operation,
							   (uint8_t *)(decrypt_msg) + olen,
							   decrypt_size - olen,
							   &olen);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_finish failed! (Error: %d)", status);
		stat = cryptoapp_err;
        goto common;
	}


	PRINT_HEX("Decrypted text", (uint8_t *)decrypt_msg, decrypt_size);


	/* Clean up cipher operation context */
	status = psa_cipher_abort(&operation);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_abort failed! (Error: %d)", status);
			stat = cryptoapp_err;
        goto common;
	}

	LOG_INF("Decryption successful!");


common:
    return stat;
}
