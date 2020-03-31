/*

ble.cpp
Description

*/
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <linux/reboot.h>
#include "common.h"
#include "Nvm.h"

QueueInfo_ST nvmQueueInfoRev;
QueueInfo_ST nvmQueueInfoSnd;

CNvm *CNvm::m_instance = NULL;

CNvm::CNvm() {

}

CNvm::~CNvm() {

}

CNvm *CNvm::GetInstance() {
  if (m_instance == NULL) {
    m_instance = new CNvm;
  }
  return m_instance;
}

void CNvm::FreeInstance() {

  if (m_instance != NULL) {
    delete m_instance;
  }

  m_instance = NULL;

}

BOOL CNvm::Init() {

  m_binit = false;

  nvm_Init();

  m_binit = CreateThread(); /*param is default time*/

  return m_binit;
}

void CNvm::Deinit() {
  KillThread();
}

void CNvm::Run() {

  InformThread();

}
BOOL CNvm::Processing() {
  int ret_len;
  mFramework = Framework::GetInstance();

  WriteFile();

  while (!m_bExit) {
#if QUEUE_TEST_MACRO
    ReceiveQueueTestTask();
     SendQueueTestTask();
    Sleep(200 * 1000);
#else
    printf("nvm processing \r\n");

    QuId_NVM_2_FW = mFramework->ID_Queue_NVM_To_FW;
    QuId_FW_2_NVM = mFramework->ID_Queue_FW_To_NVM;

//    mFramework->fw_NvmSendQueue(ID_FW_2_NVM_FWINFO);

    if (QuId_NVM_2_FW != -1 && QuId_FW_2_NVM != -1) {
      m_u8WorkMode = NVMWORK_NORMAL;
    } else {
      m_u8WorkMode = NVMWORK_NOTREADY;
      NVMLOG("[ERROR] fw not ready!!\n");
    }

    switch (m_u8WorkMode) {
      case NVMWORK_NOTREADY:NVMLOG("[NVM_PROCESS]NVMWORK_NOTREADY!!\n");
        break;

      case NVMWORK_INIT:NVMLOG("[NVM_PROCESS]NVMWORK_INIT!!\n");
        break;

      case NVMWORK_NORMAL:NVMLOG("[NVM_PROCESS]NVMWORK_NORMAL!!\n");
        NVMSyncProcess();
//        NVMSendQueue(GR_THD_SOURCE_NVM, ID_NVM_2_FW_TEST);
//        NVMRecvQueue();
        NVMRecvProcess();
        NVMSendProcess();
        NVMMonitor();

        break;
      default:break;
    }

    Sleep(NVM_THREAD_PERIOD);
#endif
  }

}

BOOL CNvm::TimeoutProcessing() {
  printf("jason add ble timeout processing \r\n");
  return true;
}

void CNvm::ReceiveQueueTestTask() {
  int len = 0;
  QueueInfo_ST QueueTest;

  if (Framework::GetInstance()->ID_Queue_FW_To_NVM > 0) {
    len = msgrcv(Framework::GetInstance()->ID_Queue_FW_To_NVM,
                 &QueueTest,
                 sizeof(QueueInfo_ST) - sizeof(long),
                 1,
                 IPC_NOWAIT);
    if (len == -1) {
      printf("jason add nvm receive from  fw no data \r\n");
      printf("jason add nvm receive from  fw no data \r\n");
      printf("jason add nvm receive from  fw no data \r\n");
    } else {
      printf("jason add nvm receive from  fw len = %d \r\n", len);
      printf("jason add nvm receive from  fw QueueTest.Msgs[0] = %x \r\n", QueueTest.Msgs[0]);
      printf("jason add nvm receive from  fw QueueTest.Msgs[1] = %x \r\n", QueueTest.Msgs[1]);
      printf("jason add nvm receive from  fw QueueTest.Msgs[2] = %x \r\n", QueueTest.Msgs[2]);
      printf("jason add nvm receive from  fw QueueTest.Msgs[3] = %x \r\n", QueueTest.Msgs[3]);
      printf("jason add nvm receive from  fw QueueTest.Msgs[4] = %x \r\n", QueueTest.Msgs[4]);
      printf("jason add nvm receive from  fw QueueTest.Msgs[500] = %x \r\n", QueueTest.Msgs[500]);
      printf("jason add nvm receive from  fw QueueTest.Msgs[501] = %x \r\n", QueueTest.Msgs[501]);
      printf("jason add nvm receive from  fw QueueTest.Msgs[502] = %x \r\n", QueueTest.Msgs[502]);
      printf("jason add nvm receive from  fw QueueTest.Msgs[503] = %x \r\n", QueueTest.Msgs[503]);
      printf("jason add nvm receive from  fw QueueTest.Msgs[503] = %x \r\n", QueueTest.Msgs[504]);
    }
  }

}

void CNvm::SendQueueTestTask() {
  QueueInfo_ST QueueTest;
  int len = 0;
  static int count = 0;
  QueueTest.mtype = 1;

  if ((Framework::GetInstance()->ID_Queue_NVM_To_FW > 0) && (count++ % 5 == 0)) {
    QueueTest.Msgs[0] = 0x66;
    QueueTest.Msgs[1] = 0x77;
    QueueTest.Msgs[2] = 0x88;
    QueueTest.Msgs[3] = 0x99;
    QueueTest.Msgs[4] = 0xaa;

    QueueTest.Msgs[500] = 0xaa;
    QueueTest.Msgs[501] = 0x99;
    QueueTest.Msgs[502] = 0x88;
    QueueTest.Msgs[503] = 0x77;
    QueueTest.Msgs[504] = 0x66;

    len = msgsnd(Framework::GetInstance()->ID_Queue_NVM_To_FW,
                 &QueueTest,
                 sizeof(QueueInfo_ST) - sizeof(long),
                 IPC_NOWAIT);
    if (len == -1) {
      printf("jason add nvm send to Framework error \r\n");

    } else {
      printf("jason add nvm send to mcu Framework success\r\n");

    }
  }

}

bool CNvm::nvm_SyncProcess() {
  static uint8_t nvmSyncCnt = 0;
  /*only need send SYNC ID*/

  nvmSyncCnt++;

  if (nvmSyncCnt >= CNT_SYNC_5S) {
    nvmSyncCnt = 0;

    nvm_SendQueue(GR_THD_SOURCE_NVM, ID_NVM_2_FW_SYNC);
  }

}
int CNvm::nvm_SendQueue(uint8_t Gr, uint8_t Id) {
  m_stSndMsg.head.Gr = Gr;
  m_stSndMsg.head.Id = Id;
  m_stSndMsg.mtype = 1;
  uint16_t Cmdlen = 0;
  int resLen = 0;

  ParseFile();

  switch (Id) {

    case ID_FW_2_NVM_SYNC:memcpy(m_stSndMsg.Msgs, &m_stDataParameter, sizeof(m_stDataParameter));
      break;
    default:break;

  }

  resLen = msgsnd(QuId_NVM_2_FW, (void *) &m_stSndMsg, sizeof(QueueInfo_ST) - sizeof(long), IPC_NOWAIT);

  if (resLen == C_FALSE) {
    NVMLOG("[ERROR]: LEN MISMATCH :cmdlen %d,reslen:%d\n", Cmdlen, resLen);
    return C_FALSE;
  } else {
    return C_TRUE;
  }

}

int CNvm::nvm_RecvQueue() {

  int readLen = 0;

  readLen = msgrcv(QuId_FW_2_NVM, (void *) &m_stRevMsg, sizeof(QueueInfo_ST) - sizeof(long), 1, IPC_NOWAIT);

  if (readLen > 0) {
    NVMLOG("[SUCCESS]RevData len %d \n", readLen);
    NVMLOG("[SUCCESS]RevData : 0x%x,%s \n", m_stRevMsg.head.Id, (char *) m_stRevMsg.Msgs);
  } else {
    NVMLOG("[WARN] no receive data !!!\n");
    return FALSE;
  }

  ParseData(m_stRevMsg.Msgs, sizeof(m_stRevMsg.Msgs));

  switch (m_stRevMsg.head.Gr) {

    case GR_THD_SOURCE_NVM:
      switch (m_stRevMsg.head.Id) {

        case ID_FW_2_NVM_CONFIG:NVMLOG("ID_FW_2_NVM_CONFIG\n");
          break;
        case ID_FW_2_NVM_SYSTEM:NVMLOG("ID_FW_2_NVM_CONFIG\n");
          break;
        case ID_FW_2_NVM_FAULT:NVMLOG("ID_FW_2_NVM_CONFIG\n");
          break;
        default:NVMLOG("default \n");
          break;

      }
      break;
    default:break;
  }
}
int CNvm::ReadFile() {

  //TODO:read file
  return 0;
}
int CNvm::WriteFile() {

  NVMLOG("wf \n");

  FILE *fp = NULL;

//  memset(&m_wFileFormat, 0, sizeof(m_wFileFormat));

  fp = fopen(SAVE_FILE_PATH, "wb+");
  if (fp == NULL) {
    NVMLOG("open file fail\n");
    return C_FALSE;
  }

  fwrite(&m_wFileFormat, sizeof(m_wFileFormat), 1, fp);

  fclose(fp);
  NVMLOG("wf finish \n");

  return C_TRUE;
}
int CNvm::ParseFile() {

  //TODO:parse file to format
  return 0;
}

int CNvm::ParseData(uint8_t *buffer, uint32_t size) {
  //TODO:parse data
  uint32_t crc = 0xffffffff;
  memset(&m_wFileFormat, 0, sizeof(m_wFileFormat));

  crc = (crc, &m_wFileFormat, sizeof(m_wFileFormat) - sizeof(m_wFileFormat.checkSum));

  return 0;
}

void CNvm::initCrcTable() {
  uint32_t c;
  uint32_t i;
  uint32_t j;

  for (i = 0; i < 256; i++) {
    c = (uint32_t) i;
    for (j = 0; j < 8; j++) {
      if (c & 1)
        c = 0xedb88320L ^ (c >> 1);
      else
        c = c >> 1;
    }
    m_u32CrcTable[i] = c;
  }
}
uint32_t CNvm::getCRC32(uint32_t crc, uint8_t *buffer, uint32_t size) {
  uint32_t i;
  for (i = 0; i < size; i++) {
    crc = m_u32CrcTable[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
  }
  return crc;
}
int CNvm::calcImgCRC(uint8_t *buffer, uint32_t *incrc) {
  int fd;
  int nread;
  int ret;
  uint8_t buf[BUFSIZE];

  uint32_t crc = 0xffffffff;

  while ((nread = read(fd, buf, BUFSIZE)) > 0) {
    crc = getCRC32(crc, buf, nread);
  }
  *incrc = crc;

  close(fd);

  if (nread < 0) {
    printf("%d:read %s.\n", __LINE__, strerror(errno));
    return -1;
  }

  return 0;
}
int CNvm::NVMMonitor() {

  //TODO：change file
  return 0;
}

int CNvm::GetInitSts() {
  return 0;
}
int CNvm::nvm_Init() {
  m_u8WorkMode = NVMWORK_NOTREADY;
  QuId_NVM_2_FW = C_FALSE;
  QuId_FW_2_NVM = C_FALSE;
  initCrcTable();



  return 0;
}

