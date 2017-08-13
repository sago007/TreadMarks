// This file is part of Tread Marks
// 
// Tread Marks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Tread Marks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Tread Marks.  If not, see <http://www.gnu.org/licenses/>.

//VoxelWorld, controlling global object for voxel terrain worlds.

#include "VoxelWorld.h"

#include "Compression.h"

#include "TextLine.hpp"

#include "EntityTank.h"
#include "EntityFlag.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//#define DISABLE_TYPE_SYNCH
//Defining this disables entity type synching.  BE SURE TO COMMENT OUT FOR FULL VERSION BUILDS!!!
//////////////////////////////////////////////////////////////////////////////////////////////////


//Transmission mode for entity create/delete notices.
#define ENTITY_TM TM_Ordered

void VoxelWorld::SetPacketPriorityDistance(float range){
	PacketPriDist = std::max(1.0f, range);
}
int32_t VoxelWorld::QueueEntityPacket(EntityBase *ent, TransmissionMode tm, BitPackEngine &bpe, float priority){
	return QueueEntityPacket(ent, tm, (char*)bpe.Data(), bpe.BytesUsed(), priority);
}
int32_t VoxelWorld::QueueEntityPacket(EntityBase *ent, TransmissionMode tm, char *data, int32_t len, float priority){
	static char buf[1024];
	if(Net.IsServerActive() && ent && data && len > 0 && len + 5 <= 1024){
		buf[0] = BYTE_HEAD_MSGENT;
		WLONG(&buf[1], ent->GID);
		memcpy(&buf[5], data, len);
		len += 5;	//Packet is now ready to ship off.
		if(tm != TM_Unreliable){
			//Must be reliable, send to all clients.
			return Net.QueueSendClient(CLIENTID_BROADCAST, tm, buf, len);
		}else{
			//Unreliable, so send to clients based on range.
			//
			return ProbabilityPacket(ent, tm, buf, len, priority);
			//
		//	for(int32_t c = 0; c < MAX_CLIENTS; c++){
		//		if(Clients[c].Connected == false) continue;	//Not connected.
		//		EntityBase *e = GetEntity(Clients[c].ClientEnt);
		//		if(!e) continue;	//Entity not found.
		//		float d = DistVec3(ent->Position, e->Position);	//Distance from client entity to entity in question.
		//		d /= std::max(0.1f, priority * PacketPriDist);	//PacketPriDist is range within updates are every frame.
		//		d = d * d;	//Square result, so faster falloff with distance.
		//		d = std::max(1.0f, d);	//d is now "every d frames, send packet".
		//		d = 1.0f / d;	//d now holds probability of packet being sent.
		//		if((float)rand() / (float)RAND_MAX < d){	//Probability says we should send packet.
		//			Net.QueueSendClient(c, tm, buf, len);
		//		}
		//	}
		//	return 1;
		}
	}
	return 0;
}
int32_t VoxelWorld::ProbabilityPacket(EntityBase *ent, TransmissionMode tm, char *data, int32_t len, float priority){
	for(int32_t c = 0; c < MAX_CLIENTS; c++){
		if(Clients[c].Connected == false) continue;	//Not connected.
		EntityBase *e = GetEntity(Clients[c].ClientEnt);
		if(!e) continue;	//Entity not found.
		float d = DistVec3(ent->Position, e->Position);	//Distance from client entity to entity in question.
		d /= std::max(0.1f, priority * PacketPriDist);	//PacketPriDist is range within updates are every frame.
	//	d = d * d;	//Square result, so faster falloff with distance.
		//
		int32_t pri = 100 - (d * 100.0f);	//Give priority boost to closest packets for Rate dropping.
		//
		d = std::max(1.0f, d);	//d is now "every d frames, send packet".
		d = 1.0f / d;	//d now holds probability of packet being sent.
		if((float)TMrandom() / (float)RAND_MAX < d){	//Probability says we should send packet.
			Net.QueueSendClient(c, tm, data, len, std::max(0, pri));
		}
	}
	return 1;
}

#define MAX_SCORCH 64
//Landscape will get no darker than 32/256ths of normal.

//Special TreadMark making function.
int32_t VoxelWorld::TreadMark(float val, int32_t x, int32_t y, int32_t dx, int32_t dy){
	if(dx == 0 && dy == 0) return 0;
	int32_t m = (int32_t)(val * 256.0f);
	int32_t count;
	int32_t dirtx, dirty, dirtw, dirth;
	if(dx < 0) dirtx = x + dx;
	else dirtx = x;
	if(dy < 0) dirty = y + dy;
	else dirty = y;
	dirtw = abs(dx);
	dirth = abs(dy);
	//
	x = (x << 8) + 128;
	y = (y << 8) + 128;
	if(abs(dx) > abs(dy)){
		count = abs(dx);
		dx = dx < 0 ? -256 : 256;
		dy = (dy <<8) / count;
	}else{
		count = abs(dy);
		dy = dy < 0 ? -256 : 256;
		dx = (dx <<8) / count;
	}
	//
	int32_t max_scorch = MAX_SCORCH;
	if(Map.GetScorchEco() >= 0) max_scorch = 0;
	//
//	unsigned char r, g, b;
	while(count--){
		int32_t ix = x >>8, iy = y >>8;
	//	map.GetRGB(ix, iy, &r, &g, &b);
	//	map.PutRGB(ix, iy, (((int32_t)r) * m) >>8, (((int32_t)g) * m) >>8, (((int32_t)b) * m) >>8);
		int32_t tc = (((int32_t)Map.GetCwrap(ix, iy)) * m) >>8;
		Map.SetCwrap(ix, iy, std::max(tc, max_scorch));
		x += dx;
		y += dy;
	}
	if(dirtw > 0 || dirth > 0){
		Map.TextureLight32(Eco, MAXTERTEX, dirtx, dirty, dirtx + dirtw + 1, dirty + dirth + 1);
		Map.UpdateTextures(dirtx, dirty, dirtw + 1, dirth + 1);
	}
	return 1;
}
int32_t VoxelWorld::Crater(int32_t cx, int32_t cy, float r, float d, float scorch){
	//
	if(Net.IsServerActive()){	//Propagate craters to clients.
		if(CraterTail){
			CraterTail = CraterTail->AddObject(new CraterEntry(cx, cy, r, d, scorch));
		}
//		char b[32];
//		b[0] = BYTE_HEAD_CRATER;
//		WSHORT(&b[1], (short)cx);
//		WSHORT(&b[3], (short)cy);
//		WFLOAT(&b[5], r);
//		WFLOAT(&b[9], d);
//		WFLOAT(&b[13], scorch);
//		Net.QueueSendClient(CLIENTID_BROADCAST, TM_Reliable, b, 17);
	}
	//
	if(!Map.Width() || !Map.Height()) return 0;	//QuickFix(tm).
	//
	//
	int32_t max_scorch = MAX_SCORCH;
	if(Map.GetScorchEco() >= 0) max_scorch = 0;
	//
	int32_t ir = (int32_t)ceil(r);
	int32_t x1 = cx - ir, x2 = cx + ir, y1 = cy - ir, y2 = cy + ir;
	float irad = 1.0f / r;
	for(int32_t y = y1; y < y2; y++){
		for(int32_t x = x1; x < x2; x++){
			float xd = (x - cx) * irad, yd = (y - cy) * irad;
			float t = 1.0f - (xd * xd + yd * yd);
		//	t = std::max(t, 0.0f);
			if(t > 0.0f){
				int32_t m = (1.0f - (1.0f - scorch) * t) * 255.0f;
				int32_t tc = (((int32_t)Map.GetCwrap(x, y)) * m) >>8;
				Map.SetCwrap(x, y, std::max(tc, max_scorch));
			//	Map.SetHwrap(x, y, Map.GetHwrap(x, y) - (int32_t)(t * d * 4.0f));
				//Do a low-pass filter at the same time.
				Map.SetHwrap(x, y,
					( ((int32_t)Map.GetHwrap(x, y) + (int32_t)Map.GetHwrap(x + 1, y) +
					(int32_t)Map.GetHwrap(x, y + 1) + (int32_t)Map.GetHwrap(x + 1, y + 1) + 1)	//Add 1 to make results biased a little more fairly towards going up.
					- (int32_t)(t * d * 16.0f) ) >>2);	//4-sample box filter, then subtract crater * 4 (meters * 16 for qtr-meters), and finally divide total by 4.
			}
		}
	}
	Map.TextureLight32(Eco, MAXTERTEX, x1, y1, x2, y2);
	Map.UpdateTextures(x1, y1, x2 - x1, y2 - y1);
	Map.MapLod(x1, y1, x2 - x1, y2 - y1);
	//Need to throw in Lod Variance recalc here too!
	//
	//Let entities know terrain has changed.  Not using Collision Bucket approach, since shrubs and other menial entities will need to know of terrain mods too.
	float radsquare = r * r, fx = (float)(Map.WrapX(cx)), fy = -(float)(Map.WrapY(cy));
	EntityBase *ent;
	for(int32_t i = 0; i < ENTITY_GROUPS; i++){
		ent = EntHead[i].NextLink();
		while(ent){
			float dx = ent->Position[0] - fx;
			float dy = ent->Position[2] - fy;
			if(dx * dx + dy * dy < radsquare) ent->TerrainModified();	//Not taking entity's bounding radius into account...
			ent = ent->NextLink();
		}
	}
	//
	return 1;
}
int32_t VoxelWorld::PackPosition(BitPackEngine &bpe, const Vec3 pos, int32_t bits){
	if(!pos) return 0;
	Vec3 tv;
	SubVec3(pos, worldcenter, tv);
	int32_t largest = 0, i = 0;
	int32_t tmp;
	for(i = 0; i < 3; i++) {tmp = abs((int32_t)(tv[i])); if(tmp > largest) largest = tmp;}
	largest /= 1000;
	int32_t xtra = 0;
	if(largest > 0) xtra = HiBit(largest) + 1;
	if(xtra <= 0){
		bpe.PackUInt(0, 1);
	}else{
		bpe.PackUInt(1, 1);
		bpe.PackUInt(xtra, 6);
	}
	float rng = (1000 * (1 <<xtra));
	for(i = 0; i < 3; i++){
		bpe.PackFloatInterval(tv[i], -rng, rng, bits + xtra);
	}
	return 1;
}
int32_t VoxelWorld::UnpackPosition(BitUnpackEngine &bpe, Vec3 pos, int32_t bits){
	if(!pos) return 0;
	Vec3 tv;
	int32_t i = 0, xtra = 0;
	bpe.UnpackUInt(i, 1);
	if(i) bpe.UnpackUInt(xtra, 6);
	float rng = (1000 * (1 <<xtra));
	for(i = 0; i < 3; i++){
		bpe.UnpackFloatInterval(tv[i], -rng, rng, bits + xtra);
	}
	AddVec3(tv, worldcenter, pos);
	return 1;
}

#define CREATEFLAGS_POS	0x01
#define CREATEFLAGS_ROT	0x02
#define CREATEFLAGS_VEL	0x04
#define CREATEFLAGS_ID	0x08
#define CREATEFLAGS_FLG	0x10

#define CrBits 16
#define CrRng1 -1000
#define CrRng2 1000
#define CrRotBits 16
#define CrRotRng1 -8
#define CrRotRng2 8
#define CrVelBits 16
#define CrVelRng1 -1000
#define CrVelRng2 1000

void VoxelWorld::SendEntityCreate(ClientID client, EntityBase *e, float transpri){
	//
	e->MirroredOnClients = true;	//Let entity know it exists on clients.
	//
	BitPacker<1024> bp;
	bp.PackInt(BYTE_HEAD_CREATEENT, 8);
	char flag = 0;
	if(e->Position[0] != 0.0f || e->Position[1] != 0.0f || e->Position[2] != 0.0f)
		flag |= CREATEFLAGS_POS;
	if(e->Rotation[0] != 0.0f || e->Rotation[1] != 0.0f || e->Rotation[2] != 0.0f)
		flag |= CREATEFLAGS_ROT;
	if(e->Velocity[0] != 0.0f || e->Velocity[1] != 0.0f || e->Velocity[2] != 0.0f)
		flag |= CREATEFLAGS_VEL;
	if(e->ID != 0)
		flag |= CREATEFLAGS_ID;
	if(e->Flags != 0)
		flag |= CREATEFLAGS_FLG;
	bp.PackUInt(flag, 8);
	bp.PackUInt(e->GID | ENTITYGID_NETWORK, 32);
	bp.PackUInt(e->TypePtr->thash, 32);
	if(flag & CREATEFLAGS_POS){
	//	for(int32_t i = 0; i < 3; i++){
	//		bp.PackFloatInterval(e->Position[i] - worldcenter[i], CrRng1, CrRng2, CrBits);
	//	}
		PackPosition(bp, e->Position, CrBits);
	}
	if(flag & CREATEFLAGS_ROT){
		for(int32_t i = 0; i < 3; i++){
			bp.PackFloatInterval(e->Rotation[i], CrRotRng1, CrRotRng2, CrRotBits);
		}
	}
	if(flag & CREATEFLAGS_VEL){
		for(int32_t i = 0; i < 3; i++){
			bp.PackFloatInterval(e->Velocity[i], CrVelRng1, CrVelRng2, CrVelBits);
		}
	}
	if(flag & CREATEFLAGS_ID){
		bp.PackUInt(e->ID, 32);
	}
	if(flag & CREATEFLAGS_FLG){
		bp.PackUInt(e->Flags, 32);
	}
	int32_t d = e->ExtraCreateInfo((unsigned char*)bp.Data() + bp.BytesUsed(), 1024 - bp.BytesUsed());	//Ask entity if it wants to pad buffer with an extra message.
	//
	if(client == CLIENTID_BROADCAST && e->TypePtr->Transitory){
		//Probabilistically throw out by distance Transitory ForceNetted entities.
		ProbabilityPacket(e, TM_Unreliable, (char*)bp.Data(), bp.BytesUsed() + d, transpri);
	}else{
		Net.QueueSendClient(client, ENTITY_TM, (char*)bp.Data(), bp.BytesUsed() + d);
	}
}
void VoxelWorld::SendEntityDelete(ClientID client, EntityBase *e){
	char buf[16];
	buf[0] = BYTE_HEAD_DELETEENT;
	WLONG(&buf[1], e->GID | ENTITYGID_NETWORK);
	Net.QueueSendClient(client, ENTITY_TM, buf, 5);
}

void VoxelWorld::Connect(ClientID source){
	//
	//Send entity dump, map name, etc.
	//
	if(source < MAX_CLIENTS){
		//Valid client ID.
		Clients[source].Init();
		Clients[source].Connected = true;
		char b = BYTE_HEAD_CONNACK;
		Net.QueueSendClient(source, TM_Ordered, &b, 1);
		//
		//Prepare entity type checksum buckets.
		for(ConfigFileList *cfgl = ConfigListHead.NextLink(); cfgl; cfgl = cfgl->NextLink()){
			cfgl->ClientChecksum[source] = 0;
		}
		//
	}
	if(source == CLIENTID_SERVER){
		Status = STAT_ConnectionAccepted;
	}
	//
	if(UCB) UCB->Connect(source);
}
void VoxelWorld::Disconnect(ClientID source, NetworkError ne){
	//
	//Remove entity associated with connection.
	//
	if(source < MAX_CLIENTS){
		//Valid client ID.
		//
		//TODO: Get rid of current client entity first.  Done.
		if(Clients[source].ClientEnt != 0){
			EntityBase *e = GetEntity(Clients[source].ClientEnt);
			if(e) e->Remove();
			char	tmpString[1024];
			sprintf(tmpString, Text.Get(TEXT_PLAYERDISCONNECTED), Clients[source].ClientName.get());
			StatusMessage(tmpString, STATUS_PRI_NETWORK);
		}
		//
		Clients[source].Init();
	}
	if(source == CLIENTID_SERVER){
		Status = STAT_Disconnect;
	}
	if(UCB) UCB->Disconnect(source, ne);
}


void VoxelWorld::PacketReceived(ClientID source, const char *data, int32_t len){
	if(len < 1) return;
	//Process packets we can...
	if(source < MAX_CLIENTS && Clients[source].Connected){
		if(data[0] == BYTE_HEAD_CONNINFO && len >= 75){
			ClassHash OldEntType = Clients[source].EntType;
			Clients[source].EntType = RLONG(&data[2]);
			int32_t OldRate = Clients[source].ClientRate;
			Clients[source].ClientRate = RLONG(&data[6]);
			Net.SetClientRate(source, Clients[source].ClientRate);
			char b[1024];
			memcpy(b, &data[11], (unsigned char)data[10]);
			b[(unsigned char)data[10]] = 0;
			CStr OldName = Clients[source].ClientName;
			Clients[source].ClientName = b;
			//
			//TODO:  Spawn, if not spawned, client entity, and set name and rate.  Think it's done...
			//
			if(data[1]){	//Respond with initial data dump.
				//
				char	tmpString[1024];
				sprintf(tmpString, Text.Get(TEXT_PLAYERISCONNECTING), Clients[source].ClientName.get());
				StatusMessage(tmpString, STATUS_PRI_NETWORK);
				//
#ifndef DISABLE_TYPE_SYNCH
				//First send any entity type files that the client doesn't have or has wrong versions of.
				int32_t tosend = 0;
				ConfigFileList *cfgl = ConfigListHead.NextLink();
				for(; cfgl; cfgl = cfgl->NextLink()){	//Count the number we will send.
					if(cfgl->ClientChecksum[source] != cfgl->GetChecksum() && cfgl->ServerSynch) tosend++;
				}
				//Now send 'em.
				int32_t num = 0;
				for(cfgl = ConfigListHead.NextLink(); cfgl; cfgl = cfgl->NextLink()){
					if(cfgl->ClientChecksum[source] != cfgl->GetChecksum() && cfgl->ServerSynch){
						num++;
						BitPacker<2048> pe;
						pe.PackUInt(BYTE_HEAD_ENTTYPE, 8);
						pe.PackUInt(num, 16);
						pe.PackUInt(tosend, 16);
					//	char buf[2048];
					//	cfgl->GetCompressedData(buf, 2048);
					//	buf[2047] = 0;
					//	pe.PackString(buf, 7);
						//ZLib Compression!!
						MemoryBuffer mem(cfgl->GetCompressedLength()), memout;
						cfgl->GetCompressedData((char*)mem.Data(), mem.Length());
						memout = ZCompress(&mem);
						uint32_t l = memout.Length();
						pe.PackUInt(l, 32);
						for(int32_t i = 0; i < l; i++){
							pe.PackUInt(*((unsigned char*)memout.Data() + i), 8);
						}
						Net.QueueSendClient(source, TM_Ordered, (char*)pe.Data(), pe.BytesUsed());
					}
				}
#else
				OutputDebugLog("\nEntity Type Synching is Disabled, Not Synching Client.\n\n");
#endif
				//
				b[0] = BYTE_HEAD_SERVINFO;
				WLONG(&b[1], gamemode);
				b[5] = (unsigned char)std::min(255, MapFileName.len() + 1);
				memcpy(&b[6], MapFileName.get(), (unsigned char)b[5]);
				b[261] = 0;
				WLONG(&b[262], mirrored);	//Send whether map is mirrored or not.
				Net.QueueSendClient(source, TM_Ordered, b, 266);
				//
				int32_t i;
				for(i = 0; i < ENTITY_GROUPS; i++){
					EntityBase *e = EntHead[i].NextLink();
					while(e){
						if(e->TypePtr->Transitory == false && e->RemoveMe == REMOVE_NONE){
							SendEntityCreate(source, e);
							//NOTE: This needs to be TM_Ordered, even if later most ent creates are Reliable...
						}
						e = e->NextLink();
					}
				}
				//
				//TODO: Also send list of map structure changes.
				//
				b[0] = BYTE_HEAD_DUMPEND;	//Let client know that initial entity dump is finished.
				Net.QueueSendClient(source, TM_Ordered, b, 1);
			}
			EntityTypeBase *et = FindEntityType(Clients[source].EntType);
			if(OldEntType != Clients[source].EntType && et){	//Make sure new type is valid before killing old entity.
				EntityBase *e = GetEntity(Clients[source].ClientEnt);
				if(e){
					e->Remove();
					Clients[source].ClientEnt = 0;
					char	tmpString[1024];
					sprintf(tmpString, Text.Get(TEXT_PLAYERCHANGEDVEHICLE), Clients[source].ClientName.get(), et->dname.get());
					StatusMessage(tmpString, STATUS_PRI_NETWORK);
				}
			}
			if(Clients[source].ClientEnt == 0){	//Not spawned yet.
				EntityBase *e = AddEntity(Clients[source].EntType, NULL, NULL, NULL, 0, 0, 0, ADDENTITY_NONET);
				if(!e){	//Try again with default entity type.
					Clients[source].EntType = DefClientEntType;
					AddEntity(Clients[source].EntType, NULL, NULL, NULL, 0, 0, 0, ADDENTITY_NONET);
				}
				if(e){
					Clients[source].ClientEnt = e->GID;
					e->SetInt(ATT_CMD_FIRSTSPAWN, true);
					//
				//	e->SetInt(ATT_CMD_FIXTANK, 0);	//UnFix, to hide until player spawns.
					//
					e->SetString(ATT_NAME, Clients[source].ClientName);	//Setting name should propogate to clients.
					//
					SendEntityCreate(CLIENTID_BROADCAST, e);	//Send create _after_ position spawned.
					//
					char buf[1024];
					buf[0] = BYTE_HEAD_MSGENT;
					WLONG(&buf[1], e->GID);
					buf[5] = (char)MSG_HEAD_PLAYERENT;	//Instruct entity _on that client_ to register as player ent.
					buf[6] = 1;
					Net.QueueSendClient(source, TM_Ordered, buf, 7);
				}
			}else{
				EntityBase *e = GetEntity(Clients[source].ClientEnt);
				if(e){
					e->SetString(ATT_NAME, Clients[source].ClientName);	//Setting name should propogate to clients.
					if(OldName != Clients[source].ClientName)
					{
						char tmpString[1024];
						sprintf(tmpString, Text.Get(TEXT_PLAYERCHANGEDNAME), OldName.get(), Clients[source].ClientName.get());
						StatusMessage(tmpString, STATUS_PRI_NETWORK);
					}
				}
				if(OldRate != Clients[source].ClientRate)
				{
					char tmpString[1024];
					sprintf(tmpString, Text.Get(TEXT_PLAYERCHANGEDBYTERATE), String(Clients[source].ClientRate).get());
					StatusMessage(tmpString, STATUS_PRI_NETWORK, source);
				}
			}
		}
		if(data[0] == BYTE_HEAD_ENTTYPE){	//This is the client->server version.
			BitUnpackEngine pe(&data[1], len - 1);
			ClassHash thash;
			uint32_t checksum;
			while(pe.UnpackUInt(thash, 32) && pe.UnpackUInt(checksum, 32)){	//For each entry in the packet.
				ConfigFileList *cfgl = ConfigListHead.NextLink();	//Find the corresponding entity type and set client's checksum of that type.
				if(cfgl && (cfgl = cfgl->Find(thash))) cfgl->ClientChecksum[source] = checksum;
			}
			return;
		}
		if(data[0] == BYTE_HEAD_TIMESYNCH && len >= 5){	//This is the client->server version.
			int32_t ctime = RLONG(&data[1]);
			char b[16];
			b[0] = BYTE_HEAD_TIMESYNCH;
			WLONG(&b[1], (msec - ctime));
			Net.QueueSendClient(source, TM_Unreliable, b, 5);	//Return difference in clocks.
			return;
		}
		if(data[0] == BYTE_HEAD_CHATMSG){	//Chat message from client to server, for distribution.
			BitUnpackEngine pe(data, len);
			pe.SkipBits(8);
			uint32_t teamid;
			CStr text;
			pe.UnpackUInt(teamid, 32);
			pe.UnpackString(text, 7);
			StatusMessage("[" + Clients[source].ClientName + "]: " + text, STATUS_PRI_CHAT, CLIENTID_BROADCAST, teamid);
			return;
		}
		if(data[0] == BYTE_HEAD_PONG && len >= 5){	//Client to server pong.
			uint32_t ptime = RLONG(&data[1]);
			EntityBase *e = GetEntity(Clients[source].ClientEnt);
			if(e) e->SetInt(ATT_PING, msec - ptime);	//Set ping value in client's ent, which will get propagated
			return;
		}
		if(UCB) UCB->PacketReceived(source, data, len);
	}
	if(source == CLIENTID_SERVER && Net.IsClientActive()){
		if(data[0] == BYTE_HEAD_MSGENT && len > 5){
			EntityBase *e = GetEntity(RLONG(&data[1]) | ENTITYGID_NETWORK);
			if(e) e->DeliverPacket((unsigned char*)&data[5], len - 5);
			return;
		}
		if(data[0] == BYTE_HEAD_CRATER){	//We gotta crater to make!
			if(CraterTail){
				//CRATER RECEIVE:
				BitUnpackEngine bp(data, len);
				bp.SkipBits(8);
				int32_t x, y;  float r, d, s;
				bp.UnpackInt(x, 13);
				bp.UnpackInt(y, 13);
				bp.UnpackFloatInterval(r, 0, 256, 11);
				bp.UnpackFloatInterval(d, -128, 128, 11);
				bp.UnpackFloatInterval(s, 0, 1.1f, 8);
				CraterTail = CraterTail->AddObject(new CraterEntry(x, y, r, d, s));
				//Now, just add it to queue, and actually make crater in think, based on ToServer ClientConnection LastCrater pointer.
			}
		//	Crater(RSHORT(&data[1]), RSHORT(&data[3]), RFLOAT(&data[5]), RFLOAT(&data[9]), RFLOAT(&data[13]));
			return;
		}
		if(data[0] == BYTE_HEAD_SERVINFO && len >= 262){
			//Load map file.
			char b[512];
			gamemode = RLONG(&data[1]);
			memcpy(b, &data[6], (unsigned char)data[5]);
			b[(unsigned char)data[5]] = 0;
			MapFileName = b;
			bool mirror = false;
			if(len >= 266){
				mirror = RLONG(&data[262]) != 0;
			}
			if(!LoadVoxelWorld(MapFileName, false, mirror) && !LoadVoxelWorld(FileNameOnly(MapFileName), false, mirror)){	//false to NOT load entities!	This will also Refresh Entity Classes.
				Status = STAT_MapLoadFailed;
			}else{
			//	ClearEntities();	//Clear loaded entities, wait for ents to come over net pipe.
			//	RefreshEntityClasses();
				Status = STAT_MapLoaded;
				//TODO:  Somewhere up here, communicate checksums of entity config files and update client list, then refresh entity classes, with another ClearEntities too.
			}
		}
		if(data[0] == BYTE_HEAD_DUMPEND){
			//Entity dump is in, so we can texture map and download textures.
			TextureVoxelWorld32();
			DownloadTextures();
			//And now the app above should be off and running.
			//
			//
			Status = STAT_RunGameLoop;
			return;
		}
		if(data[0] == BYTE_HEAD_CONNACK){
			//
#ifndef DISABLE_TYPE_SYNCH
			//Send client's entity class info before connection info.  8 bytes per entity type.
			ConfigFileList *cfgl = ConfigListHead.NextLink();
			int32_t num = 0;
			BitPacker<256> pe;
			while(cfgl){
				if(num == 0){	//Reset header in buffer.
					pe.Reset();
					pe.PackUInt(BYTE_HEAD_ENTTYPE, 8);
				}
				//XMit entity type checksum.
				pe.PackUInt(cfgl->thash, 32);
				pe.PackUInt(cfgl->GetChecksum(), 32);
				//Wrap up loop.
				cfgl = cfgl->NextLink();
				num++;
				if(cfgl == 0 || num >= 20){
					Net.QueueSendServer(TM_Ordered, (char*)pe.Data(), pe.BytesUsed());
					num = 0;
				}
			}
#else
			OutputDebugLog("\nEntity Type Synching is Disabled, Not Synching with Server.\n\n");
#endif
			//Wrap it up by sending connection info (Ordered).
			PrivSendClientInfo(true);
			Status = STAT_ConnectionInfoSent;
			return;
		}
		if(data[0] == BYTE_HEAD_ENTTYPE){	//This is the server->client version.
#ifndef DISABLE_TYPE_SYNCH
			BitUnpackEngine pe(&data[1], len - 1);
			uint32_t num = 0, total = 0, len = 0;
		//	CStr str;
			pe.UnpackUInt(num, 16);
			pe.UnpackUInt(total, 16);
		//	if(pe.UnpackString(str, 7)){
			if(pe.UnpackUInt(len, 32) && len > 0){
				MemoryBuffer mem(len), memout;
				if(mem.Data() && mem.Length() >= len){
					for(int32_t i = 0; i < len; i++){
						uint32_t n = 0;
						pe.UnpackUInt(n, 8);
						*((unsigned char*)mem.Data() + i) = n;
					}
					memout = ZUncompress(&mem);
					OutputDebugLog("Uncompressed entity type, compressed " + String((int32_t)mem.Length()) + ", original " + String((int32_t)memout.Length()) + ".\n");
				}
				//
				EntitiesToSynch = total;
				EntitiesSynched = num;
				Status = STAT_SynchEntityTypes;
				//
				ConfigFileList *cfgl = new ConfigFileList, *cfgl2;
				if(cfgl){
				//	cfgl->ReadMemBuf(str.get(), str.len() + 1);
					cfgl->ReadMemBuf((char*)memout.Data(), memout.Length());
					cfgl->Quantify();
					if(cfgl->thash != 0){
						if((cfgl2 = ConfigListHead.Find(cfgl->thash)) == 0){	//Not found already.
							ConfigListHead.AddObject(cfgl);
						}else{	//Update of existing ent.
						//	cfgl2->ReadMemBuf(str.get(), str.len() + 1);
							cfgl2->ReadMemBuf((char*)memout.Data(), memout.Length());
							cfgl2->Quantify();
							delete cfgl;
						}
					}else{	//A bad ent xmit.
						OutputDebugLog("\nBad Entity Type Transmit!\n\n");
						delete cfgl;
					}
				}
			}
#endif
			return;
		}
		if(data[0] == BYTE_HEAD_DELETEENT && len >= 5){
			EntityGID gid = RLONG(&data[1]) | ENTITYGID_NETWORK;
			EntityBase *e = GetEntity(gid);
	//	OutputDebugLog("*** Got EntityDelete, id " + String((int32_t)gid) + ", eptr " + String((int32_t)e) + ", R_F " + String(REMOVE_FORCE) + "\n");
			if(e) e->RemoveMe = REMOVE_FORCE;//DeleteItem();
			//TODO: Should add some handler for if we get a premature delete, to flag ID as already deleted.  Perhaps create dummy entity, and let collision with create bounce it.
			//ALSO: This won't work if we are for some reason doing a simultaneous client/server relay station...
			return;
		}
		if(data[0] == BYTE_HEAD_CREATEENT){// && len >= 10){
			BitUnpackEngine bp(data, len);
			bp.SkipBits(8);
			int32_t flags = 0;// = data[1];
			bp.UnpackUInt(flags, 8);
			EntityGID gid;// = RLONG(&data[2]) | ENTITYGID_NETWORK;
			bp.UnpackUInt(gid, 32);
			ClassHash hash;// = RLONG(&data[6]);	//Hashed class/type combo.
			bp.UnpackUInt(hash, 32);
			Vec3 pos = {0, 0, 0};
			Vec3 rot = {0, 0, 0};
			Vec3 vel = {0, 0, 0};
			int32_t flg = 0;
			int32_t id = 0;
			if(flags & CREATEFLAGS_POS){
				UnpackPosition(bp, pos, CrBits);
			}
			if(flags & CREATEFLAGS_ROT){
				for(int32_t n = 0; n < 3; n++){
					bp.UnpackFloatInterval(rot[n], CrRotRng1, CrRotRng2, CrRotBits);
				}
			}
			if(flags & CREATEFLAGS_VEL){
				for(int32_t n = 0; n < 3; n++){
					bp.UnpackFloatInterval(vel[n], CrVelRng1, CrVelRng2, CrVelBits);
				}
			}
			if(flags & CREATEFLAGS_ID){
				bp.UnpackUInt(id, 32);
			}
			if(flags & CREATEFLAGS_FLG){
				bp.UnpackUInt(flg, 32);
			}
			EntityBase *e = AddEntity(hash, pos, rot, vel, id, flg, gid, 0);
			if(e && (bp.BytesUsed() + 1) < len){	//Deliver extra creation data to entity.
				e->DeliverPacket((unsigned char*)&data[bp.BytesUsed()], len - bp.BytesUsed());
			}	//Note, single byte packets are disallowed for safety reasons!
			return;
		}
		if(data[0] == BYTE_HEAD_TIMESYNCH && len >= 5){	//This is the server->client version.
			int32_t msd = RLONG(&data[1]);
			if(std::abs(msd - msecdiff) > 1000){	//Whack of a jump there, probably first time.
				msecdiff = msd;
				for(int32_t n = 0; n < MSDIFF_HISTORY; n++) msecdiffa[n] = msd;
			}else{	//Filter change.
				msecdiffa[msecdiffan++] = msd;	//Add current result to history.
				msecdiffan = msecdiffan % MSDIFF_HISTORY;
				//
#define MSDINVALID 0x7fffffff
				int32_t msmin = 2000000000, msmax = -2000000000;
				int32_t n;
				for(n = 0; n < MSDIFF_HISTORY; n++){	//Find min/max.
					if(msecdiffa[n] > msmax) msmax = msecdiffa[n];
					if(msecdiffa[n] < msmin) msmin = msecdiffa[n];
				}
				double tot = 0.0;
				for(n = 0; n < MSDIFF_HISTORY; n++){	//Find average.
					if(msecdiffa[n] == msmax){
						msmax = MSDINVALID;
					}else if(msecdiffa[n] == msmin){	//Throw out min and max spikes.
						msmin = MSDINVALID;
					}else{
						tot += (double)msecdiffa[n];
					}
				}
				msecdiff = tot / (double)(MSDIFF_HISTORY - 2);
			}
		//	OutputDebugLog("MSECDIFF: " + String(msecdiff) + "  MSD: " + String(msd) + "\n");
			return;
		}
		if(data[0] == BYTE_HEAD_GAMEMODE && len >= 5){
			SetGameMode(RLONG(&data[1]));
			return;
		}
		if(data[0] == BYTE_HEAD_STATUSMSG){	//Status message from server to client.
			BitUnpackEngine pe(data, len);
			pe.SkipBits(8);
			int32_t pri = 0;
			CStr text;
			pe.UnpackUInt(pri, 8);
			pe.UnpackString(text, 7);
			StatusMessage(text, pri, CLIENTID_NONE);//, false);
			return;
		}
		if(data[0] == BYTE_HEAD_PING && len >= 5){	//Server to client ping, send pong.
			char b[16];
			b[0] = BYTE_HEAD_PONG;
			WLONG(&b[1], RLONG(&data[1]));
			Net.QueueSendServer(TM_Unreliable, b, 5);
			return;
		}
		if(UCB) UCB->PacketReceived(source, data, len);
	}
	//
	//Else...
}

void VoxelWorld::OutOfBandPacket(sockaddr_in *src, const char *data, int32_t len){
	if(UCB) UCB->OutOfBandPacket(src, data, len);
}
bool VoxelWorld::SendClientInfo(EntityTypeBase *enttype, const char *clientname, int32_t clientrate){
	if(enttype) OutgoingConnection.EntType = enttype->thash;
	if(clientname && strlen(clientname) > 0) OutgoingConnection.ClientName = clientname;
	if(clientrate > 0) OutgoingConnection.ClientRate = clientrate;
	return PrivSendClientInfo(false);
}
bool VoxelWorld::PrivSendClientInfo(bool firsttime){
	if(Net.IsClientActive()){
		unsigned char b[256];
		b[0] = BYTE_HEAD_CONNINFO;
		b[1] = firsttime;	//This is first time, we want initial data dump.
		WLONG(&b[2], OutgoingConnection.EntType);
		WLONG(&b[6], OutgoingConnection.ClientRate);
		b[10] = std::min(64, OutgoingConnection.ClientName.len() + 1);
		memcpy(&b[11], OutgoingConnection.ClientName.get(), b[10]);
		b[74] = 0;
		Net.QueueSendServer(TM_Ordered, (char*)b, 75);
		return true;
	}
	return false;
}
bool VoxelWorld::BeginClientConnect(const char *address, short serverport, short clientport, EntityTypeBase *enttype, const char *clientname, int32_t clientrate){
	OutgoingConnection.Init();
	if(address && enttype && clientname){
		OutgoingConnection.EntType = enttype->thash;
		OutgoingConnection.ClientName = clientname;
		OutgoingConnection.ClientRate = clientrate;
	//	OutgoingAddress = address;
	//	OutgoingPort = port;
		Net.FreeServer();	//For now, explicitly have one or the other...
		Net.InitClient(clientport, 20);
		Net.ClientConnect(address, serverport);
		Status = STAT_Connecting;
		return true;
	}
	return false;
}
bool VoxelWorld::InitServer(short port, int32_t maxclients){
	Net.FreeClient();
	Net.InitServer(port, std::min(maxclients, MAX_CLIENTS), 10);
	return true;
}
int32_t VoxelWorld::SetGameMode(int32_t newmode){
	int32_t t = gamemode;
	gamemode = newmode;
	//
	if(Net.IsServerActive()){
		char b[16];
		b[0] = BYTE_HEAD_GAMEMODE;
		WLONG(&b[1], gamemode);
		Net.QueueSendClient(CLIENTID_BROADCAST, TM_Reliable, b, 5);
	}
	//
	return t;
}
int32_t VoxelWorld::CountClients(){
	int32_t count = 0;
	for(int32_t i = 0; i < MAX_CLIENTS; i++){
		if(Clients[i].Connected) count++;
	}
	return count;
}
bool VoxelWorld::StatusMessage(const char *text, int32_t pri, ClientID dest, uint32_t teamid, bool localdisplay){
	if(text){
		CStr Text = text;
		if(teamid != TEAMID_NONE){
			Text = "<TEAM>" + Text;
		}
		if(dest != CLIENTID_NONE && Net.IsServerActive()){
			BitPacker<200> pe;
			pe.PackUInt(BYTE_HEAD_STATUSMSG, 8);
			pe.PackUInt(pri, 8);
			pe.PackString(Text, 7);
			for(int32_t i = 0; i < MAX_CLIENTS; i++){
				EntityBase *e = GetEntity(Clients[i].ClientEnt);
				if((dest == i || dest == CLIENTID_BROADCAST) && Clients[i].Connected &&
					e && (e->QueryInt(ATT_TEAMID) == teamid || teamid == TEAMID_NONE)){
					Net.QueueSendClient(i, TM_Reliable, (char*)pe.Data(), pe.BytesUsed());
				}
			}
		}
		if(localdisplay){
			EntityBase *e = FindRegisteredEntity("GOD");
			if(e){
				e->SetString(ATT_STATUS_MESSAGE, Text);
				e->SetInt(ATT_STATUS_PRIORITY, pri);
				OutputDebugLog(">>" + CStr(Text) + "\n", 0);	//Output to dedicated console.
				return true;
			}
		}
	}
	return false;
}
int32_t VoxelWorld::ChatMessage(const char *text, uint32_t teamid){	//Sends a chat message to the server, who sends it on to clients.
	if(text && strlen(text) > 0){
		if(!Net.IsClientActive()){//Net.IsServerActive()){
			CStr out;// = text;
			EntityBase *e = FindRegisteredEntity("PlayerEntity");
			if(e) out = "[" + e->QueryString(ATT_NAME) + "]: " + text;
			else out = CStr(Text.Get(TEXT_SERVEROPERATOR)) + " " + CStr(text);
			return StatusMessage(out, STATUS_PRI_CHAT, CLIENTID_BROADCAST, teamid);
		}else{
			BitPacker<200> pe;
			pe.PackUInt(BYTE_HEAD_CHATMSG, 8);
			pe.PackUInt(teamid, 32);
			pe.PackString(text, 7);
			Net.QueueSendServer(TM_Reliable, (char*)pe.Data(), pe.BytesUsed());
			return 1;
		}
	}
	return 0;
}
VoxelWorld::VoxelWorld() : ResourceManager(&FM), FM(), ForceGroup(false) {
	pFM = &FM;	//Just in case.
#ifndef HEADLESS
	PolyRend = &GLPolyRend;
#endif
	EntityTypeBase::SetVoxelWorld(this);
	EntityBase::SetVoxelWorld(this);
	UCB = NULL;
	Status = STAT_Disconnect;
	Net.Initialize();
	msecdiffan = 0;
	for(int32_t n = 0; n < MSDIFF_HISTORY; n++) msecdiffa[n] = 0;
	//
	CraterTail = &CraterHead;
	ClearVec3(gravity);
	ClearVec3(worldcenter);
	PacketPriDist = 50.0f;
	//
	mousex = mousey = 0.0f;
	mousebutton = 0;
	keyin = keyout = 0;
	guifocus = guiactivated = 0;
	mouseclicked = mousereleased = 0;
	//
	DefClientEntType = 0;
	//
	msec = vmsec = msecdiff = 0;
	ffrac = 0.0f;
	gamemode = 0;
	//
	mirrored = 0;
	//
	NumMajorWayPts = 0;
	//
#ifndef HEADLESS
	VoxelRend = &GLVoxelRend3;       //For real time rendering engine switching.
#endif
	//
	Net.LoadBanList("BanList.txt");
}
int32_t VoxelWorld::SelectTerrainDriver(int32_t driver){	//-1 causes a cycling.
#ifndef HEADLESS
	if(driver == -1){
		if(VoxelRend == &GLVoxelRend) VoxelRend = &GLVoxelRend2;
		else VoxelRend = &GLVoxelRend;
	}
	if(driver == 0) VoxelRend = &GLVoxelRend;
	if(driver == 1) VoxelRend = &GLVoxelRend2;
	if(driver == 2) VoxelRend = &GLVoxelRend3;
#endif

	OutputDebugLog("\nNow using terrain driver: " + CStr(VoxelRend->GLTerrainDriverName()) + "\n\n");
	return 1;
}

VoxelWorld::~VoxelWorld(){
	ClearEntities();	//Must delete entity lists while VoxelWorld services are still available!
	EntTypeHead.DeleteList();	//Clear out any existing types.
	CraterHead.DeleteList();	//Clear crater queue.
	Net.SaveBanList("BanList.txt");
}
bool VoxelWorld::DownloadTextures(bool UpdateOnly){
	bool ret = true;
	ret &= ResourceManager::DownloadTextures(UpdateOnly);
	if(UsePalTex && UpdateOnly == 0) Map.MakeTexturePalette(Eco, nTex);
	if(!UpdateOnly) ret &= Map.DownloadTextures();
	return ret;
}	//Downloads textures to OpenGL.

void VoxelWorld::UndownloadTextures(){
	ResourceManager::UndownloadTextures();
	Map.UndownloadTextures();
}

void VoxelWorld::PulseNetwork(PacketProcessorCallback *call){
	UCB = call;
	//
	if(Net.IsServerActive()){
		for(int32_t c = 0; c < MAX_CLIENTS; c++){	//Propagate craters to clients.
			if(Clients[c].Connected && Clients[c].ClientEnt){	//Once ClientEnt is set we know map init dump is done.
				if(Clients[c].LastCraterSent == NULL) Clients[c].LastCraterSent = &CraterHead;
				while(Clients[c].LastCraterSent->NextLink()){	//At least one yet to send.
					CraterEntry *ce = Clients[c].LastCraterSent->NextLink();
					//CRATER SEND:
					BitPacker<32> bp;
					bp.PackUInt(BYTE_HEAD_CRATER, 8);
					bp.PackInt(ce->x, 13);
					bp.PackInt(ce->y, 13);
					bp.PackFloatInterval(ce->r, 0, 256, 11);
					bp.PackFloatInterval(ce->d, -128, 128, 11);
					bp.PackFloatInterval(ce->scorch, 0, 1.1f, 8);
					Net.QueueSendClient(c, TM_Ordered, (char*)bp.Data(), bp.BytesUsed());	//This is an issue...  Ordered is needed since otherwise crater packets will arrive before load-the-map packet and crater queue on client will be emptied...
					Clients[c].LastCraterSent = ce;
				}
			}
		}
	}
	//
	Net.ProcessTraffic(this);
}

bool VoxelWorld::LoadEntityClasses(){
	ConfigListHead.DeleteList();
	FM.PushFile();
	FILE *f = FM.OpenWildcard("*.ent", NULL, 0, true, false);	//Find all .ent files in search path and packed files.
	if(f){
		while(f){
			ConfigFileList *cfgl = new ConfigFileList;
			if(cfgl){
				cfgl->Read(f, FM.length());
				cfgl->Quantify();
				if(ConfigListHead.Find(cfgl->cname, cfgl->tname) == NULL){	//Not found already.
					ConfigListHead.AddObject(cfgl);
				}else{	//A duplicate .ent file.
					delete cfgl;
				}
				f = FM.NextWildcard();	//Go to next .ent file.
			}
		}
		FM.PopFile();
		return RefreshEntityClasses();
	}
	FM.PopFile();
	return false;	//No entity type files found.
}
bool VoxelWorld::RefreshEntityClasses(){	//Deletes and rebuilds entity types from config file list.  Make SURE all entities are deleted and all entity classes are resource unlinked before calling this!!!
	EntTypeHead.DeleteList();	//Clear out any existing types.
	ConfigFileList *cfgl = ConfigListHead.NextLink();
	while(cfgl){
		EntityTypeBase *et = EntTypeHead.AddObject(CreateEntityType(cfgl));	//Pass to global entity type manufacturer.
		if(et){
			EntityTypeBase *et2 = FindEntityType(et->thash);
			if(et2 && et != et2){	//This is BAD!
				OutputDebugLog("*\n*\n*\nHash Collision!  " + et->cname + "/" + et->tname + " and " + et2->cname + "/" + et2->tname + "\n*\n*\n*\n");
			}
		}
		cfgl = cfgl->NextLink();
	}
	return true;
}
EntityTypeBase *VoxelWorld::FindEntityType(const char *Class, const char *Type){
	if(Class && Type && Type[0] != 0){
		EntityTypeBase *et = EntTypeHead.NextLink();
		//Check for combined "class/type" specifier in the Type parameter.
		int32_t n;
		if((n = Instr(Type, "/")) >= 0){
			CStr C = Left(Type, n);
			CStr T = Mid(Type, n + 1);
		//	OutputDebugLog("Finding special type " + C + " / " + T + "\n");
			if(et) return et->Find(C, T);
		}else{
			if(et) return et->Find(Class, Type);
		}
	}
	return 0;
}

EntityTypeBase *VoxelWorld::FindEntityType(ClassHash hash){
	EntityTypeBase *et = EntTypeHead.NextLink();
	if(et) return et->Find(hash);
	return 0;
}
int32_t VoxelWorld::EnumEntityTypes(const char *Class, EntityTypeBase **ret, int32_t cookie){	//Enumerates all Types within a Class.  Pass in 0 to begin, then the value returned from then on, until 0 is returned.  Null class string specifies ALL classes.
	if(!ret || cookie < 0) return 0;
	EntityTypeBase *et = EntTypeHead.NextLink();
	if(!et) return 0;
	et = et->FindItem(cookie);	//Return to previous location (or stay same if cookie is 0, to start).
	if(!et) return 0;
	while(et){
		cookie++;	//If first is match, cookie should point32_t to next.
		if(!Class || CmpLower(et->cname, Class)){
			*ret = et;
			return cookie;
		}
		et = et->NextLink();
	}
	return 0;
}

EntityBase *VoxelWorld::AddEntity(const char *Class, const char *Type,
								  Vec3 Pos, Rot3 Rot, Vec3 Vel, int32_t id, int32_t flags, EntityGID specificgid, int32_t AddFlags){
	return AddEntity(FindEntityType(Class, Type), Pos, Rot, Vel, id, flags, specificgid, AddFlags);
}
EntityBase *VoxelWorld::AddEntity(ClassHash hash,
								  Vec3 Pos, Rot3 Rot, Vec3 Vel, int32_t id, int32_t flags, EntityGID specificgid, int32_t AddFlags){
	return AddEntity(FindEntityType(hash), Pos, Rot, Vel, id, flags, specificgid, AddFlags);
}
EntityBase *VoxelWorld::AddEntity(EntityTypeBase *et,
								  Vec3 Pos, Rot3 Rot, Vec3 Vel, int32_t id, int32_t flags, EntityGID specificgid, int32_t AddFlags){
	if(et){
		et->CacheResources();	//Attempting support for mid-stream resource caching.
		EntityBase::SetSpecificNextGID(specificgid);
		EntityBase *ent = et->CreateEntity(Pos, Rot, Vel, id, flags);
		EntityBase::SetSpecificNextGID(0);
		if(ent){
			ent->InitialInit();
			if(!(AddFlags & ADDENTITY_NONET) && (et->Transitory == false || AddFlags & ADDENTITY_FORCENET) && Net.IsServerActive()){	//Propogate creation to clients.
				SendEntityCreate(CLIENTID_BROADCAST, ent, (AddFlags & ADDENTITY_NOSKIP ? 1000.0f : 1.0f));
			}
			return EntHead[(ForceGroup ? 0 : ent->Group())].InsertObjectAfter(ent);
			//So newer entities get added to head of list, and thus think sooner, except that Actors always think before Props, etc.
		}
	}
	return NULL;
}
EntityBase *VoxelWorld::GetEntity(EntityGID id){	//Retrieves an entity pointer by ID number.  Don't hold long, unless entity guaranteed to stay alive.
	if(id == 0) return NULL;
	EntityBucketNode *el = IDBucket.GetBucket(id);
	while(el){
		if(el->entity->GID == id) return el->entity;
		el = el->NextLink();
	}
	return NULL;
}
bool VoxelWorld::RemoveEntity(EntityBase *ent){	//Removes an entity.
	if(ent){
		ent->Remove();
		return true;
	}
	return false;
}
int32_t VoxelWorld::AddGIDBucket(EntityBucketNode *bn, EntityGID gid){
	return IDBucket.AddEntity(bn, gid);
}
bool VoxelWorld::AddPosBucket(EntityBucketNode *bn, Vec3 pos, EntityGroup grp){
	if(pos && grp < ENTITY_GROUPS && grp >= 0 && bn){
		return EntBucket[grp].AddEntity(bn, (int32_t)pos[0], (int32_t)-pos[2]);
	}
	return false;
}

int32_t VoxelWorld::CountEntities(int32_t group){
	if(group >= 0 && group < ENTITY_GROUPS){
		return EntHead[group].CountItems(-1);
	}else{
		int32_t i, count = 0;
		for(i = 0; i < ENTITY_GROUPS; i++) count += EntHead[i].CountItems(-1);
		return count;
	}
}
bool VoxelWorld::ClearEntities(){
	for(int32_t i = 0; i < ENTITY_GROUPS; i++) EntHead[i].DeleteList();
	RegEntHead.DeleteList();
	PolyRend->InitRender();	//This makes sure there are no poly render object pointers pointing into dead entities.
	UnlinkResources();
	return true;
}
bool VoxelWorld::LoadEntities(IFF *iff){
	ClearEntities();
	RefreshEntityClasses();
	return AppendEntities(iff);
}
bool VoxelWorld::AppendEntities(IFF *iff){
	char eclass[1024], etype[1024], buf[1024];
	Vec3 Pos, Vel;
	Rot3 Rot;
	int32_t i, j;
	int32_t id, flags;

	if(iff && iff->IsFileOpen() && iff->IsFileRead() && iff->FindChunk("ENTS")){
		iff->ReadLong();
		int32_t NumEnts = iff->ReadLong();
		for(i = 0; i < NumEnts; i++){
			iff->ReadBytes((uchar*)eclass, iff->ReadLong()); iff->Even();
			iff->ReadBytes((uchar*)etype, iff->ReadLong()); iff->Even();
			for(j = 0; j < 3; j++) Pos[j] = iff->ReadFloat();
			for(j = 0; j < 3; j++) Rot[j] = iff->ReadFloat();
			for(j = 0; j < 3; j++) Vel[j] = iff->ReadFloat();
			id = iff->ReadLong();
			flags = iff->ReadLong();
			iff->ReadBytes((uchar*)buf, iff->ReadLong());	//Placeholder for variable length entity specific data.
			//
			if(mirrored){
				Pos[2] = (-(Pos[2] - worldcenter[2])) + worldcenter[2];	//Mirror about horizontal center of world.
			}
			//
			AddEntity(eclass, etype, Pos, Rot, Vel, id, flags, 0, 0);
		}
		return true;
	}
	return false;
}
bool VoxelWorld::SaveEntities(IFF *iff){
	EntityTypeBase *et;
	EntityBase *ent;
	int32_t NumEnts = CountEntities();//EntHead.CountItems(-1);
	int32_t j;
	if(iff && iff->IsFileOpen() && iff->IsFileWrite()){
		iff->SetType("VOXL");	//Same as Terrain object, so they share basic file type.
		iff->StartChunk("ENTS");
		iff->WriteLong(ENTITYFILEVERSION);
		iff->WriteLong(NumEnts);
		for(int32_t i = 0; i < ENTITY_GROUPS; i++){
			ent = EntHead[i].NextLink();
			while(ent){
				et = ent->TypePtr;
				if(et){	//Write class and type names.
					iff->WriteLong(et->cname.len() + 1);
					iff->WriteBytes((uchar*)et->cname.get(), et->cname.len() + 1); iff->Even();
					iff->WriteLong(et->tname.len() + 1);
					iff->WriteBytes((uchar*)et->tname.get(), et->tname.len() + 1); iff->Even();
				}else{	//Just in case...  Write length of one null strings.
					iff->WriteLong(1); iff->WriteByte(0); iff->Even();
					iff->WriteLong(1); iff->WriteByte(0); iff->Even();
				}
				for(j = 0; j < 3; j++) iff->WriteFloat(ent->Position[j]);
				for(j = 0; j < 3; j++) iff->WriteFloat(ent->Rotation[j]);
				for(j = 0; j < 3; j++) iff->WriteFloat(ent->Velocity[j]);
				iff->WriteLong(ent->ID);
				iff->WriteLong(ent->Flags);
				iff->WriteLong(0);	//Placeholder for entity custom data storage.
				ent = ent->NextLink();
			}
		}
		iff->EndChunk();
		return true;
	}
	return false;
}
bool VoxelWorld::LoadVoxelWorld(const char *name, bool loadentities, bool mirror){
	IFF iff;
	FM.PushFile();
	if(iff.OpenIn(FM.Open(name))){
		MapFileName = name;	//So we can pass it back to clients who connect.
		FreeIndexedImages();
		if(Map.Load(&iff, &nTex, Eco, MAXTERTEX, mirror)){
			//
			mirrored = mirror;
			//
			//Set center of world.
			worldcenter[0] = (Map.Width() / 2);
			worldcenter[1] = 0.0f;
			worldcenter[2] = (-Map.Height() / 2);
			//
			InitializeEntities();
			//Try loading config text strings.
			MapCfgString = "";
			MapCfg.Free();
			if(iff.IsFileOpen() && iff.IsFileRead() && iff.FindChunk("SCFG")){
				int32_t len = iff.ReadLong();
				char *buf;
				if(len > 0 && (buf = (char*)malloc(len + 1))){
					iff.ReadBytes(buf, len);
					buf[len] = 0;
					MapCfgString = buf;	//Stick strings in CStr for easy editing and in Cfg for easy usage.
					MapCfg.ReadMemBuf(buf, len);
					free(buf);
				}
			}
			//
			if(loadentities){
				LoadEntities(&iff);	//Must do these in order, so file pointer in FM is still good!
			}else{
				ClearEntities();
				RefreshEntityClasses();
			}

			LoadIndexedFromEco();	//If this is between, it borks file pointer.
			Map.MapLod();	//Maps out areas for LOD enhancement in GL mode.
			Map.ClearCMap(255);	//Init scorch map to full color.
			FM.PopFile();
			//
			CraterHead.DeleteList();	//Clear crater queue.
			CraterTail = &CraterHead;
			//
			OutgoingConnection.LastCraterSent = NULL;	//This is needed...
			//
			EntityWaypoint *ew = (EntityWaypoint*)FindRegisteredEntity("WaypointPath0");
			if(ew)
				NumMajorWayPts = ew->CountMajorWayPoints();

			return true;
		}
	}
	FM.PopFile();
	return false;
}

int32_t VoxelWorld::ClearVoxelWorld(){
	FreeIndexedImages();
	Map.Free();
	InitializeEntities();
	ClearEntities();
	RefreshEntityClasses();
	UndownloadTextures();
	NumMajorWayPts = 0;
	return 1;
}

bool VoxelWorld::SaveVoxelWorld(const char *name){
	IFF iff;
	if(iff.OpenOut(name)){	//FileManager not used for file writes, no writing to packed files.
		if(Map.Save(&iff, Eco, nTex)){//numeco)){
			if(SaveEntities(&iff)){
				//Try to save config text strings.
				if(iff.IsFileOpen() && iff.IsFileWrite()){
					iff.StartChunk("SCFG");
					iff.WriteLong(MapCfgString.len() + 1);
					iff.WriteBytes(MapCfgString.get(), MapCfgString.len() + 1); iff.Even();
					iff.EndChunk();
				}
				return true;
			}
		}
	}
	return false;
}
bool VoxelWorld::TextureVoxelWorld(){
	if(Map.MakeShadeLookup(&InvPal)){//pe)){
		if(Map.Texture(Eco, MAXTERTEX, true)){	//Apply textures based on texid map.
			return Map.Lightsource();
		}
	}
	return false;
}
bool VoxelWorld::TextureVoxelWorld32(){
	if(Map.MakeShadeLookup(&InvPal)){
		if(Map.TextureLight32(Eco, MAXTERTEX)){
			return true;
		}
	}
	return false;
}

int32_t VoxelWorld::CacheEntityResources(){
	int32_t nents = 0;
	EntityBase *ent;
	for(int32_t i = 0; i < ENTITY_GROUPS; i++){
		ent = EntHead[i].NextLink();
		while(ent){
			nents++;
			ent->CacheResources();
			ent = ent->NextLink();
		}
	}
	return nents;
}
bool VoxelWorld::CacheResources(const char *Class, const char *Type){
	EntityTypeBase *et = FindEntityType(Class, Type);
	if(et){
		et->CacheResources();
		return true;
	}
	return false;
}
//This currently makes no note of in-world entities, which may cause problems if virtual
//EntityBase function UnlinkResources ever needs to do funky per-entity stuff...
//I'm not sure adding those functions to entity space was such a great idea actually,
//maybe I'll have to do away with them, hmm...
bool VoxelWorld::UnlinkResources(){
	EntityTypeBase *et = EntTypeHead.NextLink();
	while(et){
		et->UnlinkResources();
		et = et->NextLink();
	}
	VoxelRend->UnlinkTextures();	//Reset sky, env, etc. pointers.
	PolyRend->UnlinkTextures();
	FreeTextures();	//This assumes that ONLY ENTITIES LOAD TEXTURES!
	return true;
}

bool VoxelWorld::ListAllResources(){
	return true;
}
bool VoxelWorld::ListResources(const char *Class, const char *Type){
	EntityTypeBase *et = FindEntityType(Class, Type);
	if(et){
		et->ListResources(&CRCListHead);
		return true;
	}
	return false;
}

bool VoxelWorld::InitializeEntities(){
	IDBucket.InitBuckets(BUCKET_SIZE_ID);//, std::max(CountEntities(), 1000) * 2);
	int32_t i;//, buckets = 100;
	for(i = 0; i < ENTITY_GROUPS; i++){
		EntBucket[i].InitBuckets(Map.Width() / BUCKET_SIZE_2D, Map.Height() / BUCKET_SIZE_2D,
			BUCKET_SIZE_2D, BUCKET_SIZE_2D);//, buckets);
	//	buckets *= 10;
	}
	return true;
}

void VoxelWorld::InputTime(int32_t framemsec){
	vmsec = framemsec;
	msec += framemsec;
	ffrac = ((float)framemsec) / 1000.0f;
}
//This needs to be improved so it only thinks nernies near the camera, etc.
//
//Things are sub-optimal at the moment, especially if thousands of nernies will
//be used for shrubs and such...  Argh.  I need buckets that are updated dynamically
//with the entities they hold...
//
int32_t VoxelWorld::ThinkEntities( int32_t flags){
	FrameFlags = flags;
	int32_t nents = 0, i;
	EntityBase *ent, *ent2;
	//Add to ID/Collision buckets.
	//
	//Send time synch request packets here now.  Doing it in a consistent place should make for less jitter.
	if(Net.IsClientActive() && std::abs(int32_t(msec - lastsynch)) > SYNCH_EVERY){
		lastsynch = msec;
		char b[16];
		b[0] = BYTE_HEAD_TIMESYNCH;
		WLONG(&b[1], msec);
		Net.QueueSendServer(TM_Unreliable, b, 5);
	}
	//
	//Send pings to clients.
	if(Net.IsServerActive() && std::abs(int32_t(msec - lastping)) > PING_EVERY){
		lastping = msec;
		char b[16];
		b[0] = BYTE_HEAD_PING;
		WLONG(&b[1], msec);
		Net.QueueSendClient(CLIENTID_BROADCAST, TM_Unreliable, b, 5);
	}
	//
	CraterUpdateFlag = 0;
	if(Net.IsClientActive()){	//Actually create craters sent from server.
		if(OutgoingConnection.LastCraterSent == NULL) OutgoingConnection.LastCraterSent = &CraterHead;
		int32_t lim = 20;	//Limit number of craters actually added to map each client frame.
		while(lim > 0 && OutgoingConnection.LastCraterSent->NextLink()){
			CraterEntry *ce = OutgoingConnection.LastCraterSent->NextLink();
			Crater(ce->x, ce->y, ce->r, ce->d, ce->scorch);
			OutgoingConnection.LastCraterSent = ce;
			lim--;
		}
		if(lim <= 0){	//We are in deep crater update mode.
			CraterUpdateFlag = 1;
		}
	}
	//
	//
	for(i = 0; i < ENTITY_GROUPS; i++){
//		EntBucket[i].ClearBuckets();
		ent = EntHead[i].NextLink();
		while(ent){
			if(ent->RemoveMe){	//Entity wants to be ditched.
				ent2 = ent;
				ent = ent->NextLink();
				if(Net.IsClientActive() && !ent2->TypePtr->Transitory && ent2->RemoveMe != REMOVE_FORCE){	//REMOVE_FORCE is special case for forcing deletion of non-transitory entity on client, on receipt of kill notice.
					//Don't have authority to kill it!
				}else{
					if(Net.IsServerActive() && !ent2->TypePtr->Transitory){// && ent->RemoveMe != REMOVE_FORCE){
						//Propogate to clients.
						SendEntityDelete(CLIENTID_BROADCAST, ent2);
					}
					ent2->DeleteItem();
				}
			}else{
			//Temporarily Disabled!
//				IDBucket.AddEntity(ent, ent->GID);
			//	if(ent->CanCollide){
			//		EntBucket[i].AddEntity(ent, (int32_t)ent->Position[0], -(int32_t)ent->Position[2]);
			//	}
				ent = ent->NextLink();
			}
		}
	}

	//Think the entities.
	for(i = 0; i < ENTITY_GROUPS; i++){
		ent = EntHead[i].NextLink();
		while(ent){
			nents++;
			if(ent->CanThink){
				ent->Think();
			}
			ent = ent->NextLink();
		}
	}
	return nents;
}
EntityBase *VoxelWorld::FindRegisteredEntity(const char *name){	//Finds a registered entity by name.
	if(name){
		for(RegEntList *rl = RegEntHead.NextLink(); rl; rl = rl->NextLink()){
			if(*rl == name) return rl->entity;
		//	if(rl->cmp(name)) return rl->entity;
		}
	}
	return NULL;
}
int32_t VoxelWorld::RegisterEntity(EntityBase *ent, const char *name){	//Registers a named entity, if name not taken.
	if(!FindRegisteredEntity(name) && ent && name){
		RegEntHead.InsertObjectAfter(new RegEntList(ent, name));
		return 1;
	}else{
		return 0;
	}
}
int32_t VoxelWorld::UnregisterEntity(EntityBase *ent){
	if(ent){
		for(RegEntList *rl = RegEntHead.NextLink(); rl; rl = rl->NextLink()){
			if(rl->entity == ent){
				rl->DeleteItem();
				return 1;
			}
		}
	}
	return 0;
}

void VoxelWorld::InputMousePos(float x, float y){
	mousex = std::min(1.0f, std::max(0.0f, x));
	mousey = std::min(1.0f, std::max(0.0f, y));
}
void VoxelWorld::InputMouseButton(int32_t but){	//Use these to set mouse state from operating system.
	mousebutton = but ? 1 : 0;
}
float VoxelWorld::GetMouseX(){
	return mousex;
}
float VoxelWorld::GetMouseY(){	//GUI entities use these to access mouse state.
	return mousey;
}
int32_t VoxelWorld::GetMouseButton(){
	return mousebutton;
}
void VoxelWorld::SetChar(char c){	//Adds an ascii char to internal buffer.
	keybuf[keyin++] = c;
	if(keyin >= KEYBUF) keyin = 0;
}
void VoxelWorld::ClearChar(){	//Clears key buffer.
	keyin = keyout = 0;
}
char VoxelWorld::GetChar(){	//Used by GUI entities to read a character.
	if(keyout >= KEYBUF) keyout = 0;
	if(keyout != keyin) return keybuf[keyout++];
	return 0;
}
void VoxelWorld::SetFocus(EntityGID gid){
	guifocus = gid;
}
EntityGID VoxelWorld::GetFocus(){	//Used by gui entities to set and read input focus.
	return guifocus;
}
void VoxelWorld::SetActivated(EntityGID gid){
	guiactivated = gid;
}
EntityGID VoxelWorld::GetActivated(){	//Used by gui entities to set and read input focus.
	return guiactivated;
}
void VoxelWorld::InputMouseClicked(int32_t b){
	mouseclicked = b;
	if(b) mousereleased = 0;	//Click obliterates Release.
}
void VoxelWorld::InputMouseReleased(int32_t b){
	mousereleased = b;
}
void VoxelWorld::ClearMouseFlags(){
	mouseclicked = mousereleased = 0;
}
int32_t VoxelWorld::GetMouseClicked(){
	return mouseclicked;
}
int32_t VoxelWorld::GetMouseReleased(){
	return mousereleased;
}
void VoxelWorld::SetChatMode(int32_t mode){
	chatmode = mode;
}
int32_t VoxelWorld::GetChatMode(){
	return chatmode;
}

//All collision radii will be for CYLINDRICAL bounding volumes!

bool VoxelWorld::CheckCollision(EntityBase *ent, EntityGroup maxgroup){
	Mat43 mat1, mat2;
	int32_t mat1made = 0;
	nColliders = 0;
	if(ent && maxgroup >= 0 && maxgroup < ENTITY_GROUPS){
		//Okay, now, based on radius of calling entity's bounding sphere, scan as large a bucket area as needed.
		int32_t radlevel = (ent->BoundRad + (BUCKET_SIZE_2D / 2)) / BUCKET_SIZE_2D;
		//
		radlevel++;	//Hack to get stuck in big structures less...
		//
		int32_t bx1 = ent->Position[0] - ((BUCKET_SIZE_2D / 2) + BUCKET_SIZE_2D * radlevel);
		int32_t by1 = (-ent->Position[2]) - ((BUCKET_SIZE_2D / 2) + BUCKET_SIZE_2D * radlevel);
		int32_t bx2 = bx1 + BUCKET_SIZE_2D * (1 + radlevel * 2);
		int32_t by2 = by1 + BUCKET_SIZE_2D * (1 + radlevel * 2);
		//
		for(int32_t grp = 0; grp <= maxgroup; grp++){
			for(int32_t by = by1; by <= by2; by += BUCKET_SIZE_2D){
				for(int32_t bx = bx1; bx <= bx2; bx += BUCKET_SIZE_2D){
					EntityBucketNode *ebn = EntBucket[grp].GetBucket(bx, by);
					while(ebn){
						if(ebn->entity && ebn->entity != ent && ebn->entity->CanCollide){
							EntityBase *oe = ebn->entity;
							float rads = ent->BoundRad + oe->BoundRad;
							Vec3 delt = {oe->Position[0] - ent->Position[0], 0, oe->Position[2] - ent->Position[2]};
							if(delt[0] * delt[0] + delt[2] * delt[2] < rads * rads){
								//We have a bounding cyl intersection, possible collision.
								if(!mat1made){
									Rot3ToMat3(ent->Rotation, mat1);	//Make collider matrix.
									CopyVec3(ent->Position, mat1[3]);
									mat1made = 1;
								}
								Rot3ToMat3(oe->Rotation, mat2);	//Make collidee matrix.
								CopyVec3(oe->Position, mat2[3]);
								//
								Vec3 tmppnt;
								if( (ent->BoundMax[0] - ent->BoundMin[0]) *
									(ent->BoundMax[1] - ent->BoundMin[1]) *
									(ent->BoundMax[2] - ent->BoundMin[2]) >
									(oe->BoundMax[0] - oe->BoundMin[0]) *
									(oe->BoundMax[1] - oe->BoundMin[1]) *
									(oe->BoundMax[2] - oe->BoundMin[2]) ){	//Base pokee/poker on volume.
									if(CollideBoundingBoxes(ent->BoundMin, ent->BoundMax, mat1, oe->BoundMin, oe->BoundMax, mat2, tmppnt)){
										if(nColliders < MAX_COLLIDERS){
											Colliders[nColliders] = oe;
											Vec3MulMat43(tmppnt, mat1, ColliderPoints[nColliders]);
											nColliders++;
										}
									}
								}else{
									if(CollideBoundingBoxes(oe->BoundMin, oe->BoundMax, mat2, ent->BoundMin, ent->BoundMax, mat1, tmppnt)){
										if(nColliders < MAX_COLLIDERS){
											Colliders[nColliders] = oe;
											Vec3MulMat43(tmppnt, mat2, ColliderPoints[nColliders]);
											nColliders++;
										}
									}
								}
							}
						}
						ebn = ebn->NextLink();
					}
				}
			}
		}
		return nColliders != 0;
	}
	return false;
}

EntityBase *VoxelWorld::NextCollider(Vec3 returnpnt){
	if(nColliders > 0){
		--nColliders;
		if(returnpnt) CopyVec3(ColliderPoints[nColliders], returnpnt);
		return Colliders[nColliders];
	}
	return NULL;
}
//TODO:  Improve collisions with poly-poly test for mesh-mesh collisions.
//Do easy out against normalized space bouncing box for polies in second,
//penetrating mesh, and only continue to test polies that may be inside
//first bbox.  Then throw those against all polies in first mesh, looking
//for final collision.
//
//First bounding box gets poked by second.
int32_t CollideBoundingBoxes(Vec3 min1, Vec3 max1, Mat43 mat1, Vec3 min2, Vec3 max2, Mat43 mat2, Vec3 retpnt){
	Vec3 pnts[8] = {
	{min2[0], min2[1], min2[2]},	//Left Bottom Back
	{max2[0], min2[1], min2[2]},	//Right Bottom Back
	{min2[0], max2[1], min2[2]},	//Left Top Back
	{max2[0], max2[1], min2[2]},	//Right Top Back
	{min2[0], min2[1], max2[2]},	//Left Bottom Front
	{max2[0], min2[1], max2[2]},	//Right Bottom Front
	{min2[0], max2[1], max2[2]},	//Left Top Front
	{max2[0], max2[1], max2[2]}};	//Right Top Front
	Vec3 tpnts[8];
	static int32_t lines[12][2] = {{0, 1}, {2, 3}, {0, 2}, {1, 3}, {4, 5}, {6, 7}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
	int32_t i;
	for(i = 0; i < 8; i++){
		//TODO: This should be concatenated.
		Vec3MulMat43(pnts[i], mat2, tpnts[i]);	//Xform bbox into world space.
		Vec3IMulMat43(tpnts[i], mat1, pnts[i]);	//Back xform into bound 1 space.
	}
	Vec3 cntr;
	Vec3IMulMat3(mat2[3], mat1, cntr);	//Find 1-relative center of 2.
	Vec3 is;
	for(int32_t l = 0; l < 12; l++){
		for(int32_t d = 0; d < 3; d++){
			int32_t l1, l2;
			if(pnts[lines[l][0]][d] > pnts[lines[l][1]][d]){
				l1 = lines[l][1]; l2 = lines[l][0];
			}else{
				l1 = lines[l][0]; l2 = lines[l][1];
			}//Swap, so first line index is lowest.
			float min1d, max1d, epsi;
			if(cntr[d] < 0.0f){	//Arrange things so bb1 plane closest to bb2 center is checked first.
				min1d = min1[d];
				max1d = max1[d];
				epsi = 0.01f;
			}else{
				min1d = max1[d];
				max1d = min1[d];
				epsi = -0.01f;
			}
			if(pnts[l1][d] < min1d && pnts[l2][d] > min1d){
				float t = (min1d - pnts[l1][d]) / (pnts[l2][d] - pnts[l1][d]);
				LerpVec3(pnts[l1], pnts[l2], t + epsi, is);	//InterSect is now computed.
			}else{
				if(pnts[l1][d] < max1d && pnts[l2][d] > max1d){
					float t = (max1d - pnts[l1][d]) / (pnts[l2][d] - pnts[l1][d]);
					LerpVec3(pnts[l1], pnts[l2], t - epsi, is);	//InterSect is now computed.
				}else{
					continue;
				}
			}
			//Check InterSect against 6 planes...  Only really needs to be 4 planes.
			int32_t j;
			for(j = 0; j < 3; j++){
				if(is[j] < min1[j] || is[j] > max1[j]) break;
			}
			if(j >= 3){
				if(retpnt) CopyVec3(is, retpnt);
				return 1;	//Got through the loop, so we're in, and colliding.
			}
		}
	}
	//Sanity check for total enclosure.
	for(i = 0; i < 8; i++){
		int32_t d;
		for(d = 0; d < 3; d++){
			if(pnts[i][d] < min1[d] || pnts[i][d] > max1[d]) break;
		}
		if(d >= 3){
			if(retpnt) CopyVec3(pnts[i], retpnt);
			return 1;
		}
	}
	return 0;
}

