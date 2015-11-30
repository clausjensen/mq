#include "../MQ2Plugin.h"
#include "../moveitem.h"
  
PreSetup("MQ2Bandolier"); 
PLUGIN_VERSION(1.02); 
  
CHAR   KeyList[9]={0}; 
CHAR   szArg1[MAX_STRING]; 
CHAR   szArg2[MAX_STRING]; 
CHAR   szArg3[MAX_STRING]; 
CHAR   Command[MAX_STRING]; 
  
typedef void (__cdecl *MQ2Exchange)(PCHAR); 
  
PMQPLUGIN CheckForExchange() { 
   PMQPLUGIN pLook=pPlugins; 
   while(pLook && strnicmp(pLook->szFilename,"mq2exchange",11)) pLook=pLook->pNext; 
   return pLook; 
} 
  
MQ2Exchange GetExchangeEntry() { 
   MQ2Exchange pFind=NULL; 
   PMQPLUGIN pLook=CheckForExchange(); 
   if(pLook) pFind=(MQ2Exchange)GetProcAddress(pLook->hModule,"doExchange"); 
   return pFind; 
} 
  
MQ2Exchange GetUnequipEntry() { 
   MQ2Exchange pFind=NULL; 
   PMQPLUGIN pLook=CheckForExchange(); 
   if(pLook) pFind=(MQ2Exchange)GetProcAddress(pLook->hModule,"doUnequip"); 
   return pFind; 
} 

long FindSlotID(PCHAR ID) { 
   if(IsNumber(ID)) { 
      long Result=atoi(ID); 
      if(Result>=0 && Result<BAG_SLOT_START) return Result; 
   } else for(int i=0; i<BAG_SLOT_START; i++) { 
      if(!stricmp(ID,szItemSlot[i])) return i; 
   } 
   return -1; 
} 
  
bool FindStorage(long Size) { 
   for (int iSlot=BAG_SLOT_START; iSlot < NUM_INV_SLOTS; iSlot++) { 
      if (PCONTENTS cSlot=GetCharInfo2()->pInventoryArray->InventoryArray[iSlot]) {
         if (TypePack(cSlot) && (GetItemFromContents(cSlot)->Combine != 2) && Size <= GetItemFromContents(cSlot)->SizeCapacity) {
            if (!cSlot->pContentsArray) return true; 
            for(int iPack=0; iPack < GetItemFromContents(cSlot)->Slots; iPack++) {
               if (!cSlot->pContentsArray->Contents[iPack]) return true;
            } 
         } 
      } else return true; 
   } 
   return false; 
} 
  
void CreateSet(PSPAWNINFO pChar, PCHAR szLine) 
{ 
   int  slotNumber; 
   bool setCreated=false; 
   GetArg(szArg1,szLine,1); 
   if(szArg1[0]) { 
      WritePrivateProfileString(szArg1,NULL,NULL,INIFileName); 
       for(int i=2;i<25;i++) { 
         GetArg(szArg2,szLine,i); 
         if(!szArg2[0]) break; 
          slotNumber=FindSlotID(szArg2); 
         if(!(slotNumber<0)) { 
            sprintf(szArg2,"%d",slotNumber); 
             if(GetCharInfo2()->pInventoryArray->InventoryArray[slotNumber]) { 
               sprintf(szArg3, "%d", GetItemFromContents(GetCharInfo2()->pInventoryArray->InventoryArray[slotNumber])->ItemNumber); 
               WritePrivateProfileString(szArg1,szArg2,szArg3,INIFileName); 
                setCreated=true; 
            } 
         } 
      } 
      if(setCreated) WriteChatf("MQ2Bandolier: Set %s created",szArg1); 
   } 
   if(!setCreated) MacroError("Usage: /createset <setname> slotname1 slotname2 ... (slotname22)"); 
} 
  
void DeleteSet(PSPAWNINFO pChar, PCHAR szLine) 
{ 
   GetArg(szArg1,szLine,1); 
   if(!szArg1[0]) { 
      MacroError("Usage: /deleteset setname"); 
   } else { 
      WritePrivateProfileString(szArg1,NULL,NULL,INIFileName); 
      WriteChatf("MQ2Bandolier: Set %s deleted from ini", szArg1); 
   } 
} 
  
VOID EquipSet(PSPAWNINFO pChar, PCHAR szLine) 
{ 
   GetArg(szArg1,szLine,1); 
   if(!szArg1[0]) { 
      MacroError("Usage: /equipset setname"); 
      return; 
   } 
   CHAR EquipList[189] = {0}; 
   CHAR szSlot[5] = {0}; 
   MQ2Exchange CallExchange=GetExchangeEntry(); 
   MQ2Exchange CallUnequip=GetUnequipEntry(); 
   if(!CallExchange || !CallUnequip) { 
      if(!CheckForExchange()) MacroError("MQ2Bandolier: MQ2Exchange plugin not loaded"); 
      else MacroError("MQ2Bandolier: You need to update MQ2Exchange."); 
      return; 
   } 
   if(GetCharInfo2()->pInventoryArray->Inventory.Cursor) { 
      MacroError("MQ2Bandolier: Your mouse pointer must be clear to equip something."); 
      return; 
   } 
   if(!FindStorage(4)) { 
      MacroError("MQ2Bandolier: 1 giant size inventory slot must be free, please cleanup your bags."); 
      return; 
   } 
   GetPrivateProfileString(szArg1,NULL,"",EquipList,189,INIFileName); 
   PCHAR     pEquipList = EquipList;  
   long      ItemID;
   char      szTempItem[25] = {0};
   bool      SkipAlreadyEquipped;
   CItemLocation cItem; 
   while (pEquipList[0]!=0) { 
      GetPrivateProfileString(szArg1,pEquipList,"",szSlot,9,INIFileName); 
      if (szSlot[0]!=0) { 
         ItemID=atoi(szSlot);
         sprintf(szTempItem, "%d", ItemID);
         SkipAlreadyEquipped=false;
         if (ItemFind(&cItem, szTempItem, 0, BAG_SLOT_START)) {
            if (cItem.InvSlot != atoi(pEquipList)) {
               CallUnequip(szItemSlot[cItem.InvSlot]);
            }
            else SkipAlreadyEquipped=true;
         }
         if (!SkipAlreadyEquipped) {
            if (ItemFind(&cItem, szTempItem, BAG_SLOT_START, NUM_INV_SLOTS)) {
               PCONTENTS fItem = cItem.pBagSlot;
               if (atoi(pEquipList)==13) { 
                  long type=GetItemFromContents(fItem)->ItemType; 
                  if (type==1 || type==4 || type==35) { 
                     if (PCONTENTS sec=GetCharInfo2()->pInventoryArray->Inventory.Secondary) { 
                        CallUnequip("offhand"); 
                     } 
                  } 
               } 
               if (atoi(pEquipList)==14) { 
                  if (PCONTENTS primary=GetCharInfo2()->pInventoryArray->Inventory.Primary) { 
                     long type=GetItemFromContents(primary)->ItemType; 
                     if (type==1 || type==4 || type==35) { 
                        CallUnequip("mainhand"); 
                     } 
                  } 
               } 
               sprintf(Command,"%s %s",szSlot,pEquipList); 
               CallExchange(Command);
            }
         }    
      } 
      pEquipList+=strlen(pEquipList)+1; 
   } 
} 
  
PLUGIN_API VOID doBandolier(PCHAR szLine) { 
   EquipSet(GetCharInfo()->pSpawn,szLine); 
} 
  
PLUGIN_API VOID InitializePlugin(VOID) 
{ 
   DebugSpewAlways("Initializing MQ2Bandolier"); 
   AddCommand("/deleteset",DeleteSet); 
   AddCommand("/createset",CreateSet); 
   AddCommand("/equipset",EquipSet); 
} 
  
PLUGIN_API VOID ShutdownPlugin(VOID) 
{ 
   DebugSpewAlways("Shutting down MQ2Bandolier"); 
   RemoveCommand("/deleteset"); 
   RemoveCommand("/createset"); 
   RemoveCommand("/equipset"); 
} 
  
PLUGIN_API VOID SetGameState(DWORD GameState) 
{ 
   DebugSpewAlways("SetGameState MQ2Bandolier"); 
   if(GameState==GAMESTATE_INGAME) { 
      sprintf(INIFileName,"%s\\MQ2Bandolier_%s.ini",gszINIPath,GetCharInfo()->Name); 
   } 
} 
