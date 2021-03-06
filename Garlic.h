#ifndef GARLIC_H__
#define GARLIC_H__

#include <inttypes.h>
#include <map>
#include <list>
#include <string>
#include <thread>
#include <cryptopp/osrng.h>
#include "aes.h"
#include "I2NPProtocol.h"
#include "LeaseSet.h"
#include "Tunnel.h"
#include "Queue.h"

namespace i2p
{	
namespace garlic
{
	
	enum GarlicDeliveryType 
	{ 
		eGarlicDeliveryTypeLocal = 0, 
		eGarlicDeliveryTypeDestination = 1,
		eGarlicDeliveryTypeRouter = 2,	
		eGarlicDeliveryTypeTunnel = 3
	};	

#pragma pack(1)
	struct ElGamalBlock
	{
		uint8_t sessionKey[32];
		uint8_t preIV[32];
		uint8_t padding[158];
	};		
#pragma pack()	

	const int TAGS_EXPIRATION_TIMEOUT = 900; // 15 minutes
	class GarlicRoutingSession
	{
		public:

			GarlicRoutingSession (const i2p::data::RoutingDestination& destination, int numTags);
			~GarlicRoutingSession ();
			I2NPMessage * WrapSingleMessage (I2NPMessage * msg, const I2NPMessage * leaseSet);
			int GetNextTag () const { return m_NextTag; };
			uint32_t GetFirstMsgID () const { return m_FirstMsgID; };

			bool IsAcknowledged () const { return m_IsAcknowledged; };
			void SetAcknowledged (bool acknowledged) { m_IsAcknowledged = acknowledged; };
			
		private:

			size_t CreateAESBlock (uint8_t * buf, const I2NPMessage * msg, const I2NPMessage * leaseSet);
			size_t CreateGarlicPayload (uint8_t * payload, const I2NPMessage * msg, const I2NPMessage * leaseSet);
			size_t CreateGarlicClove (uint8_t * buf, const I2NPMessage * msg, bool isDestination);
			size_t CreateDeliveryStatusClove (uint8_t * buf, uint32_t msgID);
			
			void GenerateSessionTags ();

		private:

			const i2p::data::RoutingDestination& m_Destination;
			uint8_t m_SessionKey[32];
			uint32_t m_FirstMsgID; // first message ID
			bool m_IsAcknowledged;
			int m_NumTags, m_NextTag;
			uint8_t * m_SessionTags; // m_NumTags*32 bytes
			uint32_t m_TagsCreationTime; // seconds since epoch
			
			i2p::crypto::CBCEncryption m_Encryption;
			CryptoPP::AutoSeededRandomPool m_Rnd;
	};	

	class GarlicRouting
	{
			typedef i2p::data::Tag<32> SessionTag;
			class SessionDecryption: public i2p::crypto::CBCDecryption
			{
				public:

					SessionDecryption (): m_TagCount (0) {};
					void SetTagCount (int tagCount) { m_TagCount = tagCount; };
					void AddTagCount (int tagCount) { m_TagCount += tagCount; };
					int GetTagCount () const { return m_TagCount; };
					bool UseTag () { m_TagCount--; return m_TagCount > 0; };
					
				private:

					int m_TagCount;
			};
		
		public:

			GarlicRouting ();
			~GarlicRouting ();

			void Start ();
			void Stop ();
			void AddSessionKey (const uint8_t * key, const uint8_t * tag); // one tag 
		
			void HandleGarlicMessage (I2NPMessage * msg);
			void HandleDeliveryStatusMessage (uint8_t * buf, size_t len);
			
			I2NPMessage * WrapSingleMessage (const i2p::data::RoutingDestination& destination, I2NPMessage * msg);
			I2NPMessage * WrapMessage (const i2p::data::RoutingDestination& destination, 
			    I2NPMessage * msg, const I2NPMessage * leaseSet = nullptr);

		private:

			void Run ();
			void ProcessGarlicMessage (I2NPMessage * msg);
			void HandleAESBlock (uint8_t * buf, size_t len, SessionDecryption * decryption);
			void HandleGarlicPayload (uint8_t * buf, size_t len);
			
		private:
			
			bool m_IsRunning;
			std::thread * m_Thread;	
			i2p::util::Queue<I2NPMessage> m_Queue;
			// outgoing sessions
			std::map<i2p::data::IdentHash, GarlicRoutingSession *> m_Sessions;
			std::map<uint32_t, GarlicRoutingSession *> m_CreatedSessions; // msgID -> session
			// incoming session
			// multiple tags refer to one decyption
			std::map<SessionTag, SessionDecryption *> m_SessionTags; // tag -> decryption
	};	

	extern GarlicRouting routing;
}	
}

#endif
