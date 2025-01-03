#include "st_targetsmash.h"
#include <st/st_class_info.h>
#include <it/it_manager.h>
#include <OS/OSError.h>
#include <gm/gm_global.h>
#include <gf/gf_module.h>
#include <gf/gf_heap_manager.h>
#include <em/em_manager.h>
#include <em/em_weapon_manager.h>
#include <mt/mt_prng.h>
#include <OS/__ppc_eabi_init.h>
#include <math.h>
#include <ms/ms_message.h>
#include <wchar.h>

static stClassInfoImpl<Stages::TBreak, stTargetSmash> classInfo = stClassInfoImpl<Stages::TBreak, stTargetSmash>();

stTargetSmash* stTargetSmash::create()
{
    stTargetSmash* stage = new (Heaps::StageInstance) stTargetSmash();
    return stage;
}
bool stTargetSmash::loading()
{
    return true;
}

void stTargetSmash::update(float deltaFrame)
{
    if (!this->isItemsInitialized && itManager::getInstance()->isCompItemKindArchive(Item_Hammer, 0, true)) {
        Ground* ground = this->getGround(0);
        u32 itemsIndex = ground->getNodeIndex(0, "Items");
        u32 endIndex = ground->getNodeIndex(0, "Enemies");
        for (int i = itemsIndex + 1; i < endIndex; i++) {
            nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            this->putItem(resNodeData->m_scale.m_x, resNodeData->m_scale.m_y, resNodeData->m_scale.m_z, &resNodeData->m_translation.m_xy, resNodeData->m_translation.m_z);
        }
        this->isItemsInitialized = true;
    }

    gfModuleManager* moduleManager = gfModuleManager::getInstance();
    if (moduleManager->isLoaded("sora_enemy.rel")) {
        emManager *enemyManager = emManager::getInstance();
        if (!this->isEnemiesInitialized && enemyManager->isCompArchiveAll()) {
            Ground* ground = this->getGround(0);
            u32 itemsIndex = ground->getNodeIndex(0, "Enemies");
            u32 endIndex = ground->getNodeIndex(0, "End");
            for (int i = itemsIndex + 1; i < endIndex; i++) {
                nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
                this->putEnemy(resNodeData->m_scale.m_x, resNodeData->m_scale.m_y, resNodeData->m_scale.m_z, &resNodeData->m_translation.m_xy, resNodeData->m_translation.m_z, resNodeData->m_rotation.m_z);
            }

            this->isEnemiesInitialized = true;
        }
    }
}
void stTargetSmash::createObj()
{
    // TODO: check if target smash
    this->patchInstructions();
    // TODO: Look into switching UI to stock icon and number left if more than certain amount of targets (check IfCenter createModel functions)

    this->level = 0; // TODO

    // TODO: Check mythical Pokemon loading 1 minute in?
    // TODO: Look into using PokemonResource/AssistResource for Enemy spawns as an option if no Pokemon/Assist is loaded?

    testStageParamInit(m_fileData, 0xA);
    testStageDataInit(m_fileData, 0x14, 1);

    gfModuleManager* moduleManager = gfModuleManager::getInstance();
    int size;
    gfModuleHeader* moduleHeader = static_cast<gfModuleHeader*>(m_secondaryFileData->getData(Data_Type_Misc, 301, &size, 0xfffe));
    if (moduleHeader != NULL) {
        moduleManager->loadModuleRequestOnImage("sora_enemy.rel", Heaps::OverlayStage, moduleHeader, &size);
        emManager::create(0x1e,0x14,0);
        //gfHeapManager::dumpList();
        emWeaponManager::create();
        emWeaponManager* weaponManager = emWeaponManager::getInstance();
        weaponManager->clean();
        weaponManager->m_list1.m_last = NULL;
        weaponManager->m_list1.m_first = NULL;
        weaponManager->m_list1.m_length = 0;
        weaponManager->m_list2.m_last = NULL;
        weaponManager->m_list2.m_first = NULL;
        weaponManager->m_list2.m_length = 0;
        weaponManager->m_numStageObjects = NUM_WEAPON_STAGE_OBJECTS
        weaponManager->m_stageObjects = new (Heaps::StageInstance) wnemSimple[weaponManager->m_numStageObjects];
        for (int i = 0; i < weaponManager->m_numStageObjects; i++) {
            weaponManager->m_list1.addTail(&weaponManager->m_stageObjects[i]);
        }
        weaponManager->m_32 = false;

        emManager *enemyManager = emManager::getInstance();
        for (u32 i = 0; i < NUM_ENEMY_TYPES; i++) {
            gfArchive* brres;
            gfArchive* param;
            gfArchive* enmCommon;
            gfArchive* primFaceBrres;
            this->getEnemyPac(&brres, &param, &enmCommon, &primFaceBrres, (EnemyKind)i);
            if (brres != NULL) {
                int result = enemyManager->preloadArchive(param, brres, enmCommon, primFaceBrres, (EnemyKind) i, true);
            }
        }

    }

    this->createObjAshiba(0);
    createCollision(m_fileData, 2, NULL);

    initCameraParam();
    nw4r::g3d::ResFile posData(m_fileData->getData(Data_Type_Model, 0x64, 0xfffe));
    if (posData.ptr() == NULL)
    {
        // if no stgPos model in pac, use defaults
        createStagePositions();
    }
    else
    {
        createStagePositions(&posData);
    }
    createWind2ndOnly();
    loadStageAttrParam(m_fileData, 50);
    nw4r::g3d::ResFileData* scnData = static_cast<nw4r::g3d::ResFileData*>(m_fileData->getData(Data_Type_Scene, 0, 0xfffe));
    registScnAnim(scnData, 0);
    initPosPokeTrainer(1, 0);
    createObjPokeTrainer(m_fileData, 0x65, "PokeTrainer00", this->m_unk, 0x0);

    stTargetSmashData* stageData = static_cast<stTargetSmashData*>(this->m_stageData);
    this->setStageAttackData(&stageData->damageFloors[0], 0);
    this->setStageAttackData(&stageData->damageFloors[1], 1);
    this->setStageAttackData(&stageData->damageFloors[2], 2);

}
void stTargetSmash::createItemPac(u32 index) {
    int nodeSize;
    void* data = m_fileData->getData(Data_Type_Misc, 10001 + index, &nodeSize, 0xfffe);
    if (data != NULL) {
        gfArchive* archive = new(Heaps::StageInstance) gfArchive();
        archive->setFileImage(data, nodeSize, Heaps::StageResource);
        this->itemPacs[index] = archive;
    }
}

void stTargetSmash::createEnemyPac(u32 index) {
    int nodeSize;
    void* data = m_secondaryFileData->getData(Data_Type_Misc, index, &nodeSize, 0xfffe);
    if (data != NULL) {
        gfArchive* archive = new(Heaps::StageInstance) gfArchive();
        archive->setFileImage(data, nodeSize, Heaps::StageResource);
        this->enemyPacs[index] = archive;
    }
}

void stTargetSmash::getItemPac(gfArchive** brres, gfArchive** param, itKind itemID, int variantID, gfArchive** commonParam, itCustomizerInterface** customizer) {
    Ground* ground = this->getGround(0);
    u32 itemsIndex = ground->getNodeIndex(0, "Items");
    u32 endIndex = ground->getNodeIndex(0, "Enemies");
    for (int i = itemsIndex + 1; i < endIndex; i++) {
        nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
        if (itemID == resNodeData->m_scale.m_x && variantID == resNodeData->m_scale.m_y) {
            int index = resNodeData->m_rotation.m_x - 1;
            if (index >= 0) {
                if (this->itemPacs[index] == NULL) {
                    this->createItemPac(index);
                }
                *brres = this->itemPacs[index];
            }
            index = resNodeData->m_rotation.m_y - 1;
            if (index >= 0) {
                if (this->itemPacs[index] == NULL) {
                    this->createItemPac(index);
                }
                *param = this->itemPacs[index];
            }
            index = resNodeData->m_rotation.m_z - 1;
            if (index >= 0) {
                if (this->itemPacs[index] == NULL) {
                    this->createItemPac(index);
                }
                *commonParam = this->itemPacs[index];
            }
            return;
        }
    }
}

void stTargetSmash::getEnemyPac(gfArchive **brres, gfArchive **param, gfArchive **enmCommon, gfArchive **primFaceBrres, EnemyKind enemyID) {
    int fileIndex = enemyID * 2;
    int nodeSize;
    *primFaceBrres = NULL;

    if (this->enemyPacs[fileIndex + 1] == NULL) {
        this->createEnemyPac(fileIndex + 1);
    }
    *brres = this->enemyPacs[fileIndex + 1];

    if (this->enemyPacs[fileIndex] == NULL) {
        this->createEnemyPac(fileIndex);
    }
    *param = this->enemyPacs[fileIndex];

    if (this->enemyCommonPac == NULL) {
        void* enmCommonData = this->m_secondaryFileData->getData(Data_Type_Misc, 300, &nodeSize, (u32)0xfffe);
        *enmCommon = new (Heaps::StageInstance) gfArchive();
        (*enmCommon)->setFileImage(enmCommonData, nodeSize, Heaps::StageResource);
        this->enemyCommonPac = *enmCommon;
    }
    else {
        *enmCommon = this->enemyCommonPac;
    }

    if (*brres != NULL && (enemyID == Enemy_Prim || enemyID == Enemy_Prim_Metal || enemyID == Enemy_Prim_Big || enemyID == Enemy_Prim_Boomerang || enemyID == Enemy_Prim_SuperScope || enemyID == Enemy_Prim_Sword)) {
        if (this->primFacePac == NULL) {
            void* primFaceData = this->m_secondaryFileData->getData(Data_Type_Misc, 200 + randi(NUM_PRIM_FACES), &nodeSize, (u32)0xfffe);
            if (primFaceData == NULL) {
                primFaceData = this->m_secondaryFileData->getData(Data_Type_Misc, 200, &nodeSize, (u32)0xfffe);
            }
            *primFaceBrres = new (Heaps::StageInstance) gfArchive();
            (*primFaceBrres)->setFileImage(primFaceData, nodeSize, Heaps::StageResource);
            this->primFacePac = *primFaceBrres;
        }
        else {
            *primFaceBrres = this->primFacePac;
        }
    }
}

void stTargetSmash::notifyEventInfoGo() {
    this->applyNameCheats();
}

void stTargetSmash::applyNameCheats() {
//    for (int i = 0; i < NUM_PLAYERS; i++) {
//        int entryId = g_ftManager->getEntryId(i);
//        if (g_ftManager->isFighterActivate(entryId, -1)) {
//            Fighter* fighter = g_ftManager->getFighter(entryId, -1);
//            ftOwner* owner = g_ftmanager->getOwner(entryId);
//
//            gmPlayerInitData playerInitData = g_GameGlobal->m_modeMelee->m_playersInitData[i];
//            wchar_t name[6];
//            if (wcscmp(playerInitData.m_name, (wchar_t*)"\xFF\x2D\xFF\x25\xFF\x34\xFF\x21\xFF\x2C") == 0) {
//                //fighter->setMetal(true, 100000.0, -1);
//                owner->setMetal()
//            }
//
//        }
//    }
}
// TODO: Potential effects: shadow clone, team attack

void stTargetSmash::clearHeap() {
    for (u32 i = 0; i < NUM_ITEM_PACS; i++) {
        if (this->itemPacs[i] != NULL) {
            delete this->itemPacs[i];
        }
    }

   if (gfModuleManager::getInstance()->isLoaded("sora_enemy.rel")) {
       emWeaponManager* weaponManager = emWeaponManager::getInstance();
       if (weaponManager != NULL) {
           weaponManager->reset();
           weaponManager->m_32 = true;
           emWeaponManager::remove();
       }
       emManager* enemyManager = emManager::getInstance();
       if (enemyManager != NULL) {
           emManager* enemyManager = emManager::getInstance();
           enemyManager->removeEnemyAll();
           enemyManager->removeArchiveAll();
           emManager::remove();
       }
   }

    for (int i = 0; i < NUM_ENEMY_TYPES*2; i++) {
        if (this->enemyPacs[i] != NULL) {
            delete this->enemyPacs[i];
            this->enemyPacs[i] = NULL;
        }
    }
    if (this->enemyCommonPac != NULL) {
        delete this->enemyCommonPac;
        this->enemyCommonPac = NULL;
    }
    if (this->primFacePac != NULL) {
        delete this->primFacePac;
        this->primFacePac = NULL;
    }

    gfModuleManager::getInstance()->destroy("sora_enemy.rel");
}

void stTargetSmash::patchInstructions() {
    // stOperatorRuleTargetBreak::checkExtraRule
    // Give enough room on stack for increased number of targets

    // TODO: Use rel addresses

    int *instructionAddr = (int*)0x8095d198;
    *instructionAddr = 0x9421FC40; // stwu sp, -0x3C0(sp) Original: stwu sp, -0x60(sp)
    __flush_cache(instructionAddr - 4, 0x8);
    instructionAddr = (int*)0x8095d1a4;
    *instructionAddr = 0x900103C4; // stw r0, 0x3C4(sp) Original: stw r0, 0x64(sp)
    __flush_cache(instructionAddr - 4, 0x8);

    instructionAddr = (int*)0x8095d2e0;
    *instructionAddr = 0x800103C4; // lwz r0, 0x3C4(sp) Original: lwz r0, 0x64(sp)
    __flush_cache(instructionAddr - 4, 0x8);
    instructionAddr = (int*)0x8095d2e8;
    *instructionAddr = 0x382103C0; // addi sp, sp, 0x3C0 Original: addi sp, sp, 0x60
    __flush_cache(instructionAddr - 4, 0x8);
}

void stTargetSmash::createObjAshiba(int mdlIndex) {
    grFinal* ground = grFinal::create(mdlIndex, "", "grTargetSmashAshiba");
    if (ground != NULL)
    {
        addGround(ground);
        ground->startup(m_fileData, 0, 0);
        ground->setStageData(m_stageData);
        u32 targetsIndex = ground->getNodeIndex(0, "Targets");
        u32 disksIndex = ground->getNodeIndex(0, "Disks");
        u32 platformsIndex = ground->getNodeIndex(0, "Platforms");
        u32 slidersIndex = ground->getNodeIndex(0, "Sliders");
        u32 springsIndex = ground->getNodeIndex(0, "Springs");
        u32 cannonsIndex = ground->getNodeIndex(0, "Cannons");
        u32 laddersIndex = ground->getNodeIndex(0, "Ladders");
        u32 catapultsIndex = ground->getNodeIndex(0, "Catapults");
        u32 warpsIndex = ground->getNodeIndex(0, "Warps");
        u32 toxinsIndex = ground->getNodeIndex(0, "Toxins");
        u32 conveyorsIndex = ground->getNodeIndex(0, "Conveyors");
        u32 watersIndex = ground->getNodeIndex(0, "Waters");
        u32 windsIndex = ground->getNodeIndex(0, "Winds");
        u32 itemsIndex = ground->getNodeIndex(0, "Items");

        // TODO: Optional targets (can select max targets in STDT)
        for (int i = targetsIndex + 1; i < disksIndex; i++) {
            this->targetsLeft++;
            nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            this->createObjTarget(resNodeData->m_rotation.m_x, &resNodeData->m_translation.m_xy, &resNodeData->m_scale,
                                  resNodeData->m_translation.m_z, resNodeData->m_rotation.m_z, resNodeData->m_rotation.m_y);
        }
        for (int i = disksIndex + 1; i < platformsIndex; i++) {
            this->targetsLeft++;
            nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            this->createObjDisk(resNodeData->m_rotation.m_x, &resNodeData->m_translation.m_xy,
                                resNodeData->m_rotation.m_z, resNodeData->m_scale.m_x, resNodeData->m_scale.m_y,
                                resNodeData->m_translation.m_z, resNodeData->m_rotation.m_y, resNodeData->m_scale.m_z);
        }
        for (int i = platformsIndex + 1; i < slidersIndex; i++) {
            nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();

            switch (int(resNodeData->m_scale.m_z)) {
                case 2:
                    this->createObjBreak(resNodeData->m_rotation.m_x, &resNodeData->m_translation.m_xy,
                                         resNodeData->m_rotation.m_z, resNodeData->m_translation.m_z,
                                         resNodeData->m_rotation.m_y, resNodeData->m_scale.m_x,
                                         resNodeData->m_scale.m_y);
                    break;
                case 3:
                    this->createObjLand(resNodeData->m_rotation.m_x, &resNodeData->m_translation.m_xy,
                                        resNodeData->m_rotation.m_z, resNodeData->m_translation.m_z,
                                        resNodeData->m_rotation.m_y, resNodeData->m_scale.m_x,
                                        resNodeData->m_scale.m_y);
                    break;
                default:
                    if (resNodeData->m_scale.m_z < 0) {
                        this->createObjElevator(resNodeData->m_rotation.m_x, &resNodeData->m_translation.m_xy,
                                                &resNodeData->m_scale.m_xy, resNodeData->m_rotation.m_y,
                                                resNodeData->m_rotation.m_z, resNodeData->m_translation.m_z,
                                                -resNodeData->m_scale.m_z);
                    }
                    else {
                        this->createObjPlatform(resNodeData->m_rotation.m_x, &resNodeData->m_translation.m_xy,
                                                resNodeData->m_rotation.m_z, resNodeData->m_scale.m_x, resNodeData->m_translation.m_z,
                                                resNodeData->m_rotation.m_y);
                    }
                    break;
            }

        }
        for (int i = slidersIndex + 1; i < springsIndex; i++) {
            nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            this->createObjPunchSlider(resNodeData->m_rotation.m_x, resNodeData->m_rotation.m_y, resNodeData->m_translation.m_z,
                                       resNodeData->m_translation.m_x, resNodeData->m_translation.m_y, resNodeData->m_rotation.m_z,
                                       resNodeData->m_scale.m_x, resNodeData->m_scale.m_y, resNodeData->m_scale.m_z);
        }
        for (int i = springsIndex + 1; i < cannonsIndex; i++) {
            nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            this->createObjSpring(resNodeData->m_rotation.m_x, resNodeData->m_rotation.m_y, &resNodeData->m_translation.m_xy,
                                  resNodeData->m_rotation.m_z, &resNodeData->m_scale.m_xy, resNodeData->m_scale.m_z,
                                  resNodeData->m_translation.m_z);
        }
        for (int i = cannonsIndex + 1; i < laddersIndex; i++) {
            nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            u32 rotateFlags = resNodeData->m_scale.m_y;
            bool alwaysRotate = rotateFlags & 1;
            bool fullRotate = rotateFlags & 2;
            this->createObjCannon(resNodeData->m_rotation.m_x, &resNodeData->m_translation.m_xy,
                                  resNodeData->m_rotation.m_z, resNodeData->m_rotation.m_y, resNodeData->m_scale.m_z,
                                  resNodeData->m_translation.m_z, alwaysRotate, fullRotate, resNodeData->m_scale.m_x);
        }
        for (int i = laddersIndex + 1; i < catapultsIndex; i++) {
            nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            this->createObjLadder(resNodeData->m_rotation.m_x, &resNodeData->m_translation.m_xy, resNodeData->m_translation.m_z,
                                  resNodeData->m_rotation.m_y, resNodeData->m_rotation.m_z);
        }
        for (int i = catapultsIndex + 1; i < warpsIndex; i++) {
            nw4r::g3d::ResNodeData* resNodeData = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            this->createObjCatapult(resNodeData->m_rotation.m_x, resNodeData->m_translation.m_x,
                                    resNodeData->m_translation.m_y, resNodeData->m_translation.m_z, resNodeData->m_scale.m_z,
                                    resNodeData->m_rotation.m_y, resNodeData->m_rotation.m_z);
        }
        for (int i = warpsIndex + 1; i < toxinsIndex; i += 2) {
            nw4r::g3d::ResNodeData* resNodeDataFrom = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            nw4r::g3d::ResNodeData* resNodeDataTo = ground->m_sceneModels[0]->m_resMdl.GetResNode(i + 1).ptr();
            this->createObjWarpZone(resNodeDataFrom->m_rotation.m_x, &resNodeDataFrom->m_translation.m_xy,
                                    resNodeDataFrom->m_rotation.m_z, resNodeDataFrom->m_scale.m_z,
                                    &resNodeDataFrom->m_scale.m_xy, resNodeDataFrom->m_translation.m_z,
                                    resNodeDataFrom->m_rotation.m_y,
                                    &resNodeDataTo->m_translation.m_xy, resNodeDataTo->m_scale.m_z, resNodeDataTo->m_rotation.m_z,
                                    resNodeDataTo->m_rotation.m_x, resNodeDataTo->m_translation.m_z);
        }
        for (int i = toxinsIndex + 1; i < conveyorsIndex; i += 2) {
            nw4r::g3d::ResNodeData* resNodeDataSW = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            nw4r::g3d::ResNodeData* resNodeDataNE = ground->m_sceneModels[0]->m_resMdl.GetResNode(i + 1).ptr();
            this->createTriggerHitPointEffect(&resNodeDataSW->m_translation.m_xy, &resNodeDataNE->m_translation.m_xy,
                                              resNodeDataNE->m_scale.m_x, resNodeDataNE->m_scale.m_y,
                                              resNodeDataNE->m_rotation.m_x, resNodeDataNE->m_rotation.m_z, &resNodeDataSW->m_scale,
                                              resNodeDataNE->m_translation.m_z, resNodeDataNE->m_rotation.m_y);
        }

        for (int i = conveyorsIndex + 1; i < watersIndex; i += 2) {
            nw4r::g3d::ResNodeData* resNodeDataSW = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            nw4r::g3d::ResNodeData* resNodeDataNE = ground->m_sceneModels[0]->m_resMdl.GetResNode(i + 1).ptr();
            this->createTriggerConveyor(&resNodeDataSW->m_translation.m_xy, &resNodeDataNE->m_translation.m_xy,
                                        resNodeDataNE->m_scale.m_x, resNodeDataNE->m_scale.m_y,
                                        resNodeDataNE->m_rotation.m_x, resNodeDataNE->m_rotation.m_z, &resNodeDataSW->m_scale,
                                        resNodeDataNE->m_translation.m_z, resNodeDataNE->m_rotation.m_y);
        }
        for (int i = watersIndex + 1; i < windsIndex; i += 2) {
            nw4r::g3d::ResNodeData* resNodeDataSW = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            nw4r::g3d::ResNodeData* resNodeDataNE = ground->m_sceneModels[0]->m_resMdl.GetResNode(i + 1).ptr();
            this->createTriggerWater(&resNodeDataSW->m_translation.m_xy, &resNodeDataNE->m_translation.m_xy,
                                     resNodeDataNE->m_scale.m_x, resNodeDataNE->m_scale.m_y,
                                     resNodeDataNE->m_rotation.m_x, resNodeDataNE->m_rotation.m_z, &resNodeDataSW->m_scale,
                                     resNodeDataNE->m_translation.m_z, resNodeDataNE->m_rotation.m_y);
        }
        for (int i = windsIndex + 1; i < itemsIndex; i += 2) {
            nw4r::g3d::ResNodeData* resNodeDataSW = ground->m_sceneModels[0]->m_resMdl.GetResNode(i).ptr();
            nw4r::g3d::ResNodeData* resNodeDataNE = ground->m_sceneModels[0]->m_resMdl.GetResNode(i + 1).ptr();
            this->createTriggerWind(&resNodeDataSW->m_translation.m_xy, &resNodeDataNE->m_translation.m_xy,
                                    resNodeDataNE->m_scale.m_x, resNodeDataNE->m_scale.m_y,
                                    resNodeDataNE->m_rotation.m_x, resNodeDataNE->m_rotation.m_z, &resNodeDataSW->m_scale,
                                    resNodeDataNE->m_translation.m_z, resNodeDataNE->m_rotation.m_y);
        }
    }
}

void stTargetSmash::createObjTarget(int mdlIndex, Vec2f* pos, Vec3f* scale, int motionPathIndex, int effectIndex, int collIndex) {
    stTargetSmashData* stageData = static_cast<stTargetSmashData*>(this->m_stageData);
    grTargetSmashTarget* target = grTargetSmashTarget::create(mdlIndex, "", "grTargetSmashTarget");
    if(target != NULL) {
        addGround(target);
        target->setStageData(m_stageData);
        target->setTargetInfo(motionPathIndex, effectIndex, &this->targetsHit, &this->targetsLeft, this->numTargetsHitPerPlayer, &this->totalDamage, 0);
        if (collIndex < 0) {
            target->setupAttack(&stageData->damageFloors[(-collIndex) - 1].m_attackData);
        }
        target->startup(this->m_fileData,0,0);
        target->setPos(pos->m_x, pos->m_y, 0);
        target->setScale(scale);
        if (collIndex > 0) {
            createCollision(m_fileData, collIndex, target);
        }
    }
}

// TODO: Maybe have target setting for item only hits?

void stTargetSmash::createObjDisk(int mdlIndex, Vec2f* pos, float rot, float scaleX, float scaleZ, int motionPathIndex, int collIndex, int mode) {
    grTargetSmashDisk* disk = grTargetSmashDisk::create(mdlIndex, "", "grTargetSmashDisk");
    if(disk != NULL){
        addGround(disk);
        disk->setStageData(m_stageData);
        disk->setTargetInfo(motionPathIndex, 0, &this->targetsHit, &this->targetsLeft, this->numTargetsHitPerPlayer, &this->totalDamage, mode);
        disk->startup(this->m_fileData,0,0);
        disk->setPos(pos->m_x, pos->m_y, 0.0);
        disk->setScale(scaleX, 1.0, scaleZ);
        disk->setRot(0.0, 0.0, rot);
        createCollision(m_fileData, collIndex, disk);
    }
}

void stTargetSmash::createObjPlatform(int mdlIndex, Vec2f* pos, float rot, float scale, int motionPathIndex, int collIndex) {
    grPlatform* platform = grPlatform::create(mdlIndex, "", "grPlatform");
    if(platform != NULL){
        addGround(platform);
        platform->setStageData(m_stageData);
        platform->setMotionPathData(motionPathIndex, rot >= 360);
        platform->startup(this->m_fileData,0,0);
        platform->setPos(pos->m_x, pos->m_y, 0.0);
        platform->setScale(scale, scale, scale);
        platform->setRot(0.0, 0.0, rot);
        platform->setMotion(0);
        if (collIndex > 0) {
            createCollision(m_fileData, collIndex, platform);
        }
    }
}

void stTargetSmash::createObjBreak(int mdlIndex, Vec2f* pos, float rot, int motionPathIndex, int collIndex, float maxDamage, float respawnTime) {
    stTargetSmashData* stageData = static_cast<stTargetSmashData*>(this->m_stageData);
    grPlatform* platform = grPlatform::create(mdlIndex, "", "grBreak");
    if(platform != NULL){
        addGround(platform);
        platform->setStageData(m_stageData);
        platform->setMotionPathData(motionPathIndex, rot >= 360);
        platform->startup(this->m_fileData,0,0);
        platform->setupHitPoint(maxDamage, respawnTime);
        if (collIndex < 0) {
            platform->setupAttack(&stageData->damageFloors[(-collIndex) - 1].m_attackData);
        }
        platform->initializeEntity();
        platform->startEntity();
        platform->setPos(pos->m_x, pos->m_y, 0.0);
        platform->setRot(0.0, 0.0, rot);
        platform->setMotion(0);
        if (collIndex > 0) {
            createCollision(m_fileData, collIndex, platform);
        }
    }
}

void stTargetSmash::createObjLand(int mdlIndex, Vec2f* pos, float rot, int motionPathIndex, int collIndex, float maxLandings, float respawnTime) {
    grPlatform* platform = grPlatform::create(mdlIndex, "", "grPlatform");
    if(platform != NULL){
        addGround(platform);
        platform->setStageData(m_stageData);
        platform->setMotionPathData(motionPathIndex, rot >= 360);
        platform->startup(this->m_fileData,0,0);
        platform->setupLanding(maxLandings, respawnTime);
        platform->setPos(pos->m_x, pos->m_y, 0.0);
        platform->setRot(0.0, 0.0, rot);
        platform->setMotion(0);
        createCollision(m_fileData, collIndex, platform);
    }
}

void stTargetSmash::createObjElevator(int mdlIndex, Vec2f* pos, Vec2f* range, int collIndex, int posIndex, float speed, float deltaSpeed) {
    grAdventureElevator* elevator = grAdventureElevator::create(mdlIndex, "grAdventureElevator");
    if (elevator != NULL)
    {
        addGround(elevator);
        elevator->prepareElevatorData(pos, range, speed, deltaSpeed, posIndex);
        elevator->startup(m_fileData, 0, 0);
        createCollision(m_fileData, collIndex, elevator);
    }
}

void stTargetSmash::createObjPunchSlider(int mdlIndex, int sliderPathIndex, int motionPathIndex, float unk1, float unk2, float unk3, float unk4, float unk5, float unk6) {
    grPunchSlider* slider = grPunchSlider::create(mdlIndex, "grPunchSlider");
    if (slider != NULL) {
        addGround(slider);
        slider->setStageData(m_stageData);
        slider->prepareSliderData(motionPathIndex, sliderPathIndex, unk1, unk2, unk3, unk4, unk5, unk6);
        slider->startup(m_fileData, 0, 0);
    }
}

void stTargetSmash::createObjSpring(int mdlIndex, int collIndex, Vec2f* pos, float rot, Vec2f* range, float bounce, int motionPathIndex) {
    grSpring* spring = grSpring::create(mdlIndex, "grSpring");
    if (spring != NULL) {
        addGround(spring);
        grGimmickSpringData springData(
                pos,
                rot,
                bounce,
                mdlIndex,
                &(Vec2f){0.0, 0.0},
                range);
        spring->setMotionPathData(motionPathIndex, rot >= 360);
        spring->setGimmickData(&springData); // Note: gimmickData will only apply in next function since was allocated on the stack
        spring->startup(this->m_fileData,0,0);
        this->createGimmickCollision(collIndex, spring, this->m_fileData);
    }
}

void stTargetSmash::createObjCannon(int mdlIndex, Vec2f* pos, float rot, float rotSpeed, float maxRot, int motionPathIndex, bool alwaysRotate, bool fullRotate, float autoFireFrames) {

    grAdventureBarrelCannon* cannon = grAdventureBarrelCannon::create(mdlIndex, BarrelCannon_Static, "grAdventureBarrelCannon");
    if (cannon != NULL)
    {
        addGround(cannon);
        cannon->setStageData(m_stageData);
        cannon->prepareCannonData(pos, rot, rotSpeed, maxRot, motionPathIndex, alwaysRotate, fullRotate, autoFireFrames);
        cannon->startup(m_fileData, 0, 0);
    }
}

void stTargetSmash::createObjLadder(int mdlIndex, Vec2f* pos, int motionPathIndex, bool restrictUpExit, bool unk2) {

    grLadder* ladder = grLadder::create(mdlIndex, "grLadder");
    if (ladder != NULL) {
        addGround(ladder);
        grGimmickLadderData ladderData(
                mdlIndex, 0, restrictUpExit, unk2, "",
                &(Vec2f){0.0, 0.0}, &(Vec2f){0.0, 0.0}
                );
        ladder->setMotionPathData(motionPathIndex);
        ladder->startupLadder(this->m_fileData,0,0,&ladderData);
        ladder->setPos(pos->m_x, pos->m_y, 0.0);
    }
}

void stTargetSmash::createObjCatapult(int mdlIndex, float vector, float motionRatio, int motionPathIndex, float framesBeforeStartMove, float unk1, float unk2) {
    grCatapult* catapult = grCatapult::create(mdlIndex, "grCatapult");
    if (catapult != NULL) {
        addGround(catapult);
        catapult->prepareCatapultData(vector, motionRatio, motionPathIndex, framesBeforeStartMove, unk1, unk2);
        catapult->startup(m_fileData, 0, 0);
    }
}

void stTargetSmash::createObjWarpZone(int mdlIndex, Vec2f* pos, float rot, float scale, Vec2f* range, int motionPathIndex, float deactivateFrames, Vec2f* warpDest, u8 warpType, bool isNotAuto, int connectedMdlIndex, int connectedMotionPathIndex) {
    grWarpZone* warpZone = grWarpZone::create(mdlIndex, "grWarpZone");
    if (warpZone != NULL) {
        addGround(warpZone);
        grGimmickWarpData warpData(
                pos, mdlIndex, snd_se_ADVstage_common_FIGHTER_IN, snd_se_invalid,
                &(Vec2f){0.0, 0.0}, range
                );

        warpZone->setStageData(m_stageData);
        warpZone->prepareWarpData(motionPathIndex, deactivateFrames, rot >= 360);
        warpZone->setWarpAttrData(&(Vec3f){warpDest->m_x, warpDest->m_y, 0.0}, warpType, isNotAuto);
        warpZone->setGimmickData(&warpData); // Note: gimmickData will only apply in next function since was allocated on the stack
        warpZone->startup(m_fileData, 0, 0);
        warpZone->setRot(0, 0, rot);
        warpZone->setScale(scale, scale, scale);
        if (connectedMdlIndex > 0) {
            grWarpZone* toWarpZone = grWarpZone::create(connectedMdlIndex, "grWarpZone");
            if (toWarpZone != NULL) {
                warpData.m_pos = *warpDest;
                addGround(toWarpZone);
                toWarpZone->setStageData(m_stageData);
                toWarpZone->prepareWarpData(connectedMotionPathIndex, deactivateFrames, rot >= 360);
                toWarpZone->setWarpAttrData(&(Vec3f){pos->m_x, pos->m_y, 0.0}, warpType, isNotAuto);
                toWarpZone->setGimmickData(&warpData); // Note: gimmickData will only apply in next function since was allocated on the stack
                toWarpZone->startup(m_fileData, 0, 0);
                toWarpZone->setRot(0, 0, rot);
                toWarpZone->setScale(scale, scale, scale);

                warpZone->setConnectedWarp(toWarpZone);
                toWarpZone->setConnectedWarp(warpZone);
            }
        }
    }
}

void stTargetSmash::createTriggerHitPointEffect(Vec2f* posSW, Vec2f* posNE, float damage, short detectionRate, int mdlIndex, float rot, Vec3f* scale, int motionPathIndex, int collIndex) {
    Vec2f pos = {0.5*(posSW->m_x + posNE->m_x), 0.5*(posSW->m_y + posNE->m_y)};

    grGimmickHitPointEffectData hitPointEffectData(
            fabsf(damage),
            damage < 0 ? true : false,
            detectionRate,
            &pos,
            &(Vec2f){posNE->m_x - posSW->m_x, posNE->m_y - posSW->m_y}
            );

    stTrigger* trigger = g_stTriggerMng->createTrigger(Gimmick::Area_HitPoint_Effect, -1);
    trigger->setHitPointEffectTrigger(&hitPointEffectData);

    if (mdlIndex > 0) {
        grArea* ground = grArea::create(mdlIndex, "", "grAreaHitPointEffect");
        if (ground != NULL) {
            addGround(ground);
            ground->setTrigger(trigger);
            ground->setMotionPathData(motionPathIndex, rot >= 360);
            ground->setStageData(m_stageData);
            ground->startup(this->m_fileData,0,0);
            ground->setPos(pos.m_x, pos.m_y, 0.0);
            ground->setRot(0.0, 0.0, rot);
            ground->setScale(scale);
            if (collIndex > 0) {
                createCollision(m_fileData, collIndex, ground);
            }
        }
    }
}

void stTargetSmash::createTriggerConveyor(Vec2f* posSW, Vec2f* posNE, float speed, bool isRightDirection, int mdlIndex, float rot, Vec3f* scale, int motionPathIndex, int collIndex) {
    Vec3f pos = {0.5*(posSW->m_x + posNE->m_x), 0.5*(posSW->m_y + posNE->m_y), 0.0};

    grGimmickBeltConveyorData beltConveyorAreaData(
            &pos,
            speed,
            isRightDirection,
            &(Vec2f){0.0, 0.0},
            &(Vec2f){posNE->m_x - posSW->m_x, posNE->m_y - posSW->m_y},
            gfArea::Shape_Rectangle
            );

    stTrigger* trigger = g_stTriggerMng->createTrigger(Gimmick::Area_BeltConveyor, -1);
    trigger->setBeltConveyorTrigger(&beltConveyorAreaData);

    if (mdlIndex > 0) {
        grArea* ground = grArea::create(mdlIndex, "", "grAreaBeltConveyor");
        if (ground != NULL) {
            addGround(ground);
            ground->setTrigger(trigger);
            ground->setMotionPathData(motionPathIndex, rot >= 360);
            ground->setStageData(m_stageData);
            ground->startup(this->m_fileData,0,0);
            ground->setPos(pos.m_x, pos.m_y, 0.0);
            ground->setRot(0.0, 0.0, rot);
            ground->setScale(scale);
            if (collIndex > 0) {
                createCollision(m_fileData, collIndex, ground);
            }
        }
    }
}

void stTargetSmash::createTriggerWater(Vec2f* posSW, Vec2f* posNE, float speed, bool canDrown, int mdlIndex, float rot, Vec3f* scale, int motionPathIndex, int collIndex) {
    Vec2f pos = {0.5*(posSW->m_x + posNE->m_x), 0.5*(posSW->m_y + posNE->m_y)};

    grGimmickWaterData waterAreaData(
            posNE->m_y,
            canDrown,
            speed,
            &pos,
            &(Vec2f){posNE->m_x - posSW->m_x, posNE->m_y - posSW->m_y}
            );

    stTrigger* trigger = g_stTriggerMng->createTrigger(Gimmick::Area_Water, -1);
    trigger->setWaterTrigger(&waterAreaData);

    if (mdlIndex > 0) {
        grArea* ground = grArea::create(mdlIndex, "", "grAreaWater");
        if (ground != NULL) {
            addGround(ground);
            ground->setTrigger(trigger);
            ground->setMotionPathData(motionPathIndex, rot >= 360);
            ground->setStageData(m_stageData);
            ground->startup(this->m_fileData,0,0);
            ground->setPos(pos.m_x, pos.m_y, 0.0);
            ground->setRot(0.0, 0.0, rot);
            ground->setScale(scale);
            if (collIndex > 0) {
                createCollision(m_fileData, collIndex, ground);
            }
        }
    }
}

void stTargetSmash::createTriggerWind(Vec2f* posSW, Vec2f* posNE, float strength, float angle, int mdlIndex, float rot, Vec3f* scale, int motionPathIndex, int collIndex) {
    Vec3f pos = {0.5*(posSW->m_x + posNE->m_x), 0.5*(posSW->m_y + posNE->m_y), 0.0};

    grGimmickWindData windAreaData(
                &pos,
                strength,
                angle,
                &(Vec2f){0.0, 0.0},
                &(Vec2f){posNE->m_x - posSW->m_x, posNE->m_y - posSW->m_y}
                );
    stTrigger* trigger = g_stTriggerMng->createTrigger(Gimmick::Area_Wind, -1);
    trigger->setWindTrigger(&windAreaData);

    if (mdlIndex > 0) {
        grArea* ground = grArea::create(mdlIndex, "", "grAreaWind");
        if (ground != NULL) {
            addGround(ground);
            ground->setTrigger(trigger);
            ground->setMotionPathData(motionPathIndex, rot >= 360);
            ground->setStageData(m_stageData);
            ground->startup(this->m_fileData,0,0);
            ground->setPos(pos.m_x, pos.m_y, 0.0);
            ground->setRot(0.0, 0.0, rot);
            ground->setScale(scale);
            if (collIndex > 0) {
                createCollision(m_fileData, collIndex, ground);
            }
        }
    }
}

void stTargetSmash::putItem(int itemID, u32 variantID, int startStatus, Vec2f* pos, int motionPathIndex) {
    itManager *itemManager = itManager::getInstance();
    BaseItem *item = itemManager->createItem((itKind) itemID, variantID, -1, 0, 0, 0xffff, 0, 0xffff);
    if (item != NULL) {
        Vec3f warpPos = (Vec3f){pos->m_x, pos->m_y, 0.0};
        item->warp(&warpPos);
        item->setVanishMode(false);
        if (startStatus > 1) {
            item->changeStatus(startStatus);
        }
        if (motionPathIndex != 0) {
            grItem* ground = grItem::create(motionPathIndex, "MoveNode", "grItem", item->m_instanceId);
            if (ground != NULL) {
                addGround(ground);
                ground->startup(m_fileData, 0, 0);
                ground->startMove();
            }
        }
    }
}

void stTargetSmash::putEnemy(int enemyId, int difficulty, int startStatus, Vec2f* pos, int motionPathIndex, float lr) {
    // TODO: MotionPath index investigate if can make every enemy follow it?

    emManager* enemyManager = emManager::getInstance();

    emCreate create;
    create.m_teamNo = 10000;
    create.m_difficulty = difficulty % 15;
    create.m_enemyKind = (EnemyKind)enemyId;
    create.m_startStatusKind = startStatus;

    create.m_startPos = (Vec3f){pos->m_x, pos->m_y, 0.0};

    create.m_startLr = lr;
    create.m_level = 1 + difficulty / 15;
    create.m_36 = 0.0;
    create.m_territoryRange = (Rect2D){0.0, 0.0, 0.0, 0.0};
    create.m_connectedTriggerId = 0;

    create.m_motionPath = NULL;
    if (motionPathIndex != 0) {
        grMotionPath* ground = grMotionPath::create(motionPathIndex, "MoveNode", "grMotionPath");
        if (ground != NULL) {
            addGround(ground);
            ground->startup(m_fileData, 0, 0);
        }
        create.m_motionPath = ground;
    }
    create.m_epbm = NULL;
    create.m_epsp = NULL;
    create.m_parentCreateId = 0xFFFF;
    //OSReport("Preload archive count result: %d \n", enemyManager->getPreloadArchiveCountFromKind(Enemy_Kuribo));

    int id = enemyManager->createEnemy(&create);
    Enemy* enemy = enemyManager->getEnemyPtrFromId(id);
    enemy->m_moduleAccesser->getCameraModule()->setEnableCamera(0, -1);

    // TODO: Change death to use similar explosion as fighter ko
    // TODO: Fix death so that 2p doesn't get hit by it
}

void Ground::setStageData(void* stageData)
{
    this->m_stageData = stageData;
}
void stTargetSmash::startFighterEvent()
{
    return;
}
int stTargetSmash::initializeFighterAttackRatio()
{
    return 0;
}
int stTargetSmash::helperStarWarp()
{
    return 0;
}
bool stTargetSmash::isSimpleBossBattleMode()
{
    return false;
}
bool stTargetSmash::isBossBattleMode()
{
    return false;
}
bool stTargetSmash::isCameraLocked()
{
    return true;
}
void stTargetSmash::notifyTimmingGameStart()
{
    return;
}
float stTargetSmash::getFrameRuleTime()
{
    return this->m_frameRuleTime;
}
void stTargetSmash::setFrameRuleTime(float newTime)
{
    this->m_frameRuleTime = newTime;
}
bool stTargetSmash::isNextStepBgmEqualNowStepBgmFromFlag()
{
    return false;
}
float stTargetSmash::getBgmPlayOffsetFrame()
{
    return BGM_PLAY_OFFSET_FRAME;
}
float stTargetSmash::getBgmVolume()
{
    return BGM_VOLUME;
}
void stTargetSmash::setBgmChange(float unk, u32 unk1, u32 unk2)
{
    this->m_unk2 = unk1;
    this->m_unk3 = unk2;
    this->m_unk4 = unk;
}
void stTargetSmash::getBgmChangeID(u32 unk1, float unk2)
{
    unk1 = this->m_unk3;
    unk2 = this->m_unk4;
}
bool stTargetSmash::isBgmChange()
{
    return this->m_unk2;
}
int stTargetSmash::getBgmOptionID()
{
    return 0;
}
int stTargetSmash::getNowStepBgmID()
{
    return 0;
}
int stTargetSmash::getBgmID()
{
    return 0;
}
int stTargetSmash::getBgmIDOverload()
{
    return 0;
}
void stTargetSmash::appearanceFighterLocal()
{
    return;
}
// TODO
// stubbed because i can't figure out where this is called
int stTargetSmash::getScrollDir(u32 unk1)
{
    return 0;
}
int stTargetSmash::getDefaultLightSetIndex()
{
    return 0x14;
}
Rect2D* stTargetSmash::getAIRange()
{
    return &this->m_aiRange;
}
bool stTargetSmash::isAdventureStage()
{
    return false;
}
int stTargetSmash::getPokeTrainerDrawLayer()
{
    return 0;
}
float stTargetSmash::getPokeTrainerPositionZ()
{
    return POKETRAINER_Z;
}
void stTargetSmash::getPokeTrainerPointData(int* unk, int unk2)
{
    return;
}
int stTargetSmash::getPokeTrainerPointNum()
{
    return 0;
}
bool stTargetSmash::isReStartSamePoint()
{
    return true;
}
grGimmickWindData2nd* stTargetSmash::getWind2ndOnlyData()
{
    return m_windAreaData2nd;
}
bool stTargetSmash::isBamperVector()
{
    return true;
}
int stTargetSmash::getFinalTechniqColor()
{
    return 0x14000496;
}

ST_CLASS_INFO;