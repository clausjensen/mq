#include "../MQ2Plugin.h"
#include "../moveitem.h"

const char*  PLUGIN_NAME = "MQ2Exchange";
const double PLUGIN_VER  = 2.25;

PreSetup(PLUGIN_NAME);
PLUGIN_VERSION(PLUGIN_VER);

bool bPendingEx                 = false;
bool bPendingUn                 = false;
unsigned long ulTimer           = 0;
char szPend[MAX_STRING]         = {0};
char szPendSwap[MAX_STRING]     = {0};
char szTemp[MAX_STRING]         = {0};

void Help()
{
    WriteChatf("\ay%s Commands\ax: ", PLUGIN_NAME);
    WriteChatf(" /exchange list - Displays the list of slot names and slot numbers");
    WriteChatf(" /exchange <itemname|ID> <slotname|slotnumber> - Exchanges item to slot");
    WriteChatf(" /unequip <slotname|slotnumber> - Unequips item from slot");
}

void List()
{
    WriteChatf("\ay%s Item Slots\ax: ", PLUGIN_NAME);
    int i = 0;
    for (i = 0; szItemSlot[i]; i++)
    {
        WriteChatf("%s | %d", szItemSlot[i], i);
    }
}

bool CheckValidExchange(CItemLocation* pValidate, long lDestSlot)
{
    PCONTENTS pcontBagSlot = pValidate->pBagSlot;
    PITEMINFO pitemBagSlot = GetItemFromContents(pValidate->pBagSlot);
    PITEMINFO pitemSwapIn  = GetItemFromContents(pValidate->pItem);

    // if non-bard casting, fail
    if (NonBardCasting())
    {
        MacroError("Exchange: Cannot /exchange while casting");
        return false;
    }

    // if moving something to main inventory slots, no other checks required
    if (lDestSlot >= BAG_SLOT_START && lDestSlot < NUM_INV_SLOTS) return true;


    // if the slot contains a bag
    if (TypePack(pcontBagSlot) && GetCharInfo2()->pInventoryArray && GetCharInfo2()->pInventoryArray->InventoryArray[lDestSlot])
    {
        // check if item size is too large for that bag
        PITEMINFO pDestSlot = GetItemFromContents(GetCharInfo2()->pInventoryArray->InventoryArray[lDestSlot]);
        if (pDestSlot && pitemBagSlot && pDestSlot->Size > pitemBagSlot->SizeCapacity)
        {
            MacroError("Exchange: %s is too large to fit in %s", pDestSlot->Name, pitemBagSlot->Name);
            return false;
        }
    }

    // if the destination is the AMMO slot
    if (lDestSlot == 22)
    {
        // verify the item being exchanged is ammo-usable
        if ((pitemSwapIn->EquipSlots&(1 << 11)) || (pitemSwapIn->EquipSlots&(1 << 22)))
        {
            return true;
        }
        else
        {
            MacroError("Exchange: Cannot equip %s in the ammo slot.", pitemSwapIn->Name);
            return false;
        }
    }

    // if the destination is primary, and there is something in the secondary, and the item being moved is type 2H
    if (lDestSlot == 0xd && GetCharInfo2()->pInventoryArray->InventoryArray[0xe] && ((pitemSwapIn->ItemType == 0x1) || (pitemSwapIn->ItemType == 0x4))) // 1&4 = 2h items.  missing 35?
    {
        WriteChatf("Exchange: Cannot equip %s when %s is in the offhand slot", pitemSwapIn->Name, GetItemFromContents(GetCharInfo2()->pInventoryArray->InventoryArray[0xe])->Name);
        return false;
    }

    // if our class cannot use this item
    if (!(pitemSwapIn->Classes&(1 << ((GetCharInfo2()->Class) - 1))))
    {
        MacroError("Exchange: Cannot equip %s. Class restriction.", pitemSwapIn->Name);
        //return false;
    }

    // if our race cannot use this item
    unsigned long myRace = GetCharInfo2()->Race;
    switch(myRace)
    {
    case 0x80:
        myRace=0xc;
        break;
    case 0x82:
        myRace=0xd;
        break;
    case 0x14a:
        myRace=0xe;
        break;
    case 0x20a:
        myRace=0xf;
        break;
    default:
        myRace--;
    }
    if (!(pitemSwapIn->Races&(1 << myRace)))
    {
        MacroError("Exchange: Cannot equip %s. Race restriction.", pitemSwapIn->Name);
        return false;
    }

    /*if(pitemSwapIn->RequiredLevel>GetCharInfo2()->Level) {
    MacroError("Exchange: Cannot equip %s. Required level higher than your level",pitemSwapIn->Name);
    return false;
    }*/

    // if our deity is incorrect
    unsigned long Deity = GetCharInfo2()->Deity - 200;
    if ((pitemSwapIn->Diety != 0) && !(pitemSwapIn->Diety&(1 << Deity)))
    {
        MacroError("Exchange: Cannot equip %s. Deity restriction.", pitemSwapIn->Name);
        return false;
    }

    // exchange is valid
    return true;
}

void ExchangeCmd(PSPAWNINFO pLPlayer, char* szLine)
{
    char szArg1[MAX_STRING] = {0};
    char szArg2[MAX_STRING] = {0};
    GetArg(szArg1, szLine, 1);
    GetArg(szArg2, szLine, 2);

    if (!szArg1[0] || !szArg2[0])
    {
        MacroError("Usage: /exchange <itemname|itemID> <slotname|slotnumber>");
        CloseBags();
        return;
    }

    // check destination slot provided is valid
    long lSFSlot = SlotFind(szArg2);
    if (lSFSlot < 0)
    {
        MacroError("Exchange: %s slot not found", szArg2);
        CloseBags();
        return;
    }

    // find the item we are attempting to exchange
    CItemLocation cItem;
    if (!ItemFind(&cItem, szArg1))
    {
        MacroError("Exchange: Couldn't find %s in your inventory", szArg1);
        CloseBags();
        return;
    }

    // if the item is already worn
    if (cItem.InvSlot < BAG_SLOT_START)
    {
        // run another pass to see if we can find a copy within packs (for same name/ID)
        if (!ItemFind(&cItem, szArg1, BAG_SLOT_START))
        {
            WriteChatf("\ay%s\aw:: Item is already in a worn slot.", PLUGIN_NAME);
            CloseBags();
            return;
        }
    }

    // if the item is in the dest slot || validexchange fails
    if (cItem.InvSlot == lSFSlot)
    {
        WriteChatf("\ay%s\aw:: Item is already in desired slot.", PLUGIN_NAME);
        CloseBags();
        return;
    }
    if (!CheckValidExchange(&cItem, lSFSlot))
    {
        CloseBags();
        return;
    }

    SendInvClick(cItem.pEQInvSlot, SHIFTKEY);  // pick up the item to move
    sprintf(szTemp, "InvSlot%d", lSFSlot);
    SendWornClick(szTemp, SHIFTKEY); // swap item with worn slot (updates appearance)
    SendInvClick(cItem.pEQInvSlot, SHIFTKEY);  // place the item on cursor into the old item's place
    CloseBags();
}

void UnequipCmd(PSPAWNINFO pLPlayer, char* szLine)
{
    char szArg1[MAX_STRING] = {0};
    GetArg(szArg1, szLine, 1);

    if (!*szArg1)
    {
        MacroError("Usage: /unequip <slotname|slotnumber>");
        CloseBags();
        return;
    }

    if (NonBardCasting())
    {
        MacroError("Unequip: Cannot /unequip while casting");
        CloseBags();
        return;
    }

    // check destination slot provided is valid
    long lSFSlot = SlotFind(szArg1);
    if (lSFSlot < 0)
    {
        MacroError("Exchange: %s slot not found", szArg1);
        CloseBags();
        return;
    }

    // verify the item is in a worn slot
    if (lSFSlot >= BAG_SLOT_START)
    {
        MacroError("Cannot unequip from that slot");
        CloseBags();
        return;
    }

    PCONTENTS pUnequipSlot = GetCharInfo2()->pInventoryArray->InventoryArray[lSFSlot];
    if (!pUnequipSlot)
    {
        MacroError("Unequip: There is nothing in the %s slot to unequip", szLine);
        CloseBags();
        return;
    }

    // attempt to find an open slot
    CItemLocation cFreeSlot;
    if (!PackFind(&cFreeSlot, pUnequipSlot))
    {
        WriteChatf("\ay%s\aw:: No room in any bags to unequip.", PLUGIN_NAME);
        CloseBags();
        return;
    }

    // we have found a place to remove to, move the item
    sprintf(szTemp, "InvSlot%d", lSFSlot);
    SendWornClick(szTemp, SHIFTKEY); // pick the item up off of the slot
    //else put it in desired place
    SendInvClick(cFreeSlot.pEQInvSlot, SHIFTKEY);
    // close bags
    CloseBags();
}

PLUGIN_API void SetGameState(unsigned long ulGameState)
{
    FindBagKeys();
    if (GetGameState() != GAMESTATE_INGAME)
    {
        bPendingEx = bPendingUn = false;
        ulTimer = 0;
    }
}

PLUGIN_API void ExchangeDelayedCmd(PSPAWNINFO pLPlayer, char* szLine)
{
    if (!GetCharInfo2()) return;

    char szArg1[MAX_STRING] = {0};
    char szArg2[MAX_STRING] = {0};
    GetArg(szArg1, szLine, 1);
    GetArg(szArg2, szLine, 2);

    if (!strnicmp(szArg1, "list", 4) || !strnicmp(szArg2, "list", 4))
    {
        List();
        return;
    }
    else if (!strnicmp(szArg1, "help", 4))
    {
        Help();
        return;
    }
    else if (CursorHasItem())
    {
        MacroError("Exchange: Your mouse pointer must be clear to move an item.");
        return;
    }

    // mq2bagwindow support
    if (!BagWndLoaded())
    {
        if (bPendingEx || bPendingUn)
        {
            WriteChatf("\ay%s\aw:: Another command is already pending.", PLUGIN_NAME);
            return;
        }
        OpenBags();
        ulTimer  = GetTickCount() + MOVE_DELAY;
        bPendingEx = true;
        strcpy(szPend, szLine);
    }
    else
    {
        ExchangeCmd((PSPAWNINFO)pLocalPlayer, szLine);
    }
}

void UnequipDelayedCmd(PSPAWNINFO pLPlayer, char* szLine)
{
    if (!GetCharInfo2()) return;

    char szArg1[MAX_STRING] = {0};
    char szArg2[MAX_STRING] = {0};
    GetArg(szArg1, szLine, 1);
    GetArg(szArg2, szLine, 2);

    if (!strnicmp(szArg1, "list", 4) || !strnicmp(szArg2, "list", 4))
    {
        List();
        return;
    }
    else if (!strnicmp(szArg1, "help", 4))
    {
        Help();
        return;
    }
    else if (CursorHasItem())
    {
        MacroError("Exchange: Your mouse pointer must be clear to move an item.");
        return;
    }

    // support for mq2bagwindow
    if (!BagWndLoaded())
    {
        if (bPendingEx || bPendingUn)
        {
            WriteChatf("\ay%s\aw:: Another command is already pending.", PLUGIN_NAME);
            return;
        }
        OpenBags();
        ulTimer  = GetTickCount() + MOVE_DELAY;
        bPendingUn = true;
        strcpy(szPend, szLine);
    }
    else
    {
        UnequipCmd((PSPAWNINFO)pLocalPlayer, szLine);
    }
}

PLUGIN_API void OnPulse()
{
    if (GetGameState() == GAMESTATE_INGAME)
    {
        if (bPendingEx && GetTickCount() > ulTimer)
        {
            ExchangeCmd((PSPAWNINFO)pLocalPlayer, szPend);
            bPendingEx = false;
            ulTimer = 0;
        }
        if (bPendingUn && GetTickCount() > ulTimer)
        {
            UnequipCmd((PSPAWNINFO)pLocalPlayer, szPend);
            bPendingUn = false;
            ulTimer = 0;
        }
    }
}

PLUGIN_API void doExchange(char* szLine)
{
    if (!BagWndLoaded())
    {
        WriteChatf("\arERROR: %s exports currently require MQ2BagWindow", PLUGIN_NAME);
        return;
    }
    ExchangeDelayedCmd((PSPAWNINFO)pLocalPlayer, szLine);
}

PLUGIN_API void doUnequip(char* szLine)
{
    if (!BagWndLoaded())
    {
        WriteChatf("\arERROR: %s exports currently require MQ2BagWindow", PLUGIN_NAME);
        return;
    }
    UnequipDelayedCmd((PSPAWNINFO)pLocalPlayer, szLine);
}

PLUGIN_API void InitializePlugin()
{
    AddCommand("/exchange", ExchangeDelayedCmd);
    AddCommand("/unequip",  UnequipDelayedCmd);
}

PLUGIN_API void ShutdownPlugin()
{
    RemoveCommand("/exchange");
    RemoveCommand("/unequip");
}
