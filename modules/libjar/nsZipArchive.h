/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998-1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Daniel Veditz <dveditz@netscape.com>
 *   Samir Gehani <sgehani@netscape.com>
 *   Mitch Stoltz <mstoltz@netscape.com>
 *   Taras Glek <tglek@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsZipArchive_h_
#define nsZipArchive_h_

#define ZIP_MAGIC     0x5A49505FL   /* "ZIP_" */
#define ZIPFIND_MAGIC 0x5A495046L   /* "ZIPF" */
#define ZIP_TABSIZE   256
// Keep this odd. The -1 is significant.
#define ZIP_BUFLEN    (4 * 1024 - 1)

#define PL_ARENA_CONST_ALIGN_MASK 7
#include "plarena.h"
#define ZIP_Seek(fd,p,m) (PR_Seek((fd),((PROffset32)p),(m))==((PROffset32)p))

#include "zlib.h"
#include "nsAutoPtr.h"

class nsZipFind;
class nsZipReadState;
class nsZipItemMetadata;

struct PRFileDesc;

/**
 * This file defines some of the basic structures used by libjar to
 * read Zip files. It makes use of zlib in order to do the decompression.
 *
 * A few notes on the classes/structs:
 * nsZipArchive   represents a single Zip file, and maintains an index
 *                of all the items in the file.
 * nsZipItem      represents a single item (file) in the Zip archive.
 * nsZipFind      represents the metadata involved in doing a search,
 *                and current state of the iteration of found objects.
 * 'MT''safe' reading from the zipfile is performed through JARInputStream,
 * which maintains its own file descriptor, allowing for multiple reads 
 * concurrently from the same zip file.
 */

/**
 * nsZipItem -- a helper struct for nsZipArchive
 *
 * each nsZipItem represents one file in the archive and all the
 * information needed to manipulate it.
 */
struct nsZipItem
{
  nsZipItem*  next;

  PRUint32    headerOffset;
  PRUint32    dataOffset;
  PRUint32    size;             /* size in original file */
  PRUint32    realsize;         /* inflated size */
  PRUint32    crc32;

  /*
   * Keep small items together, to avoid overhead.
   */
  PRUint16     time;
  PRUint16     date;
  PRUint16     mode;
  PRUint8      compression;
  PRPackedBool hasDataOffset : 1;
  PRPackedBool isDirectory : 1; 
  PRPackedBool isSynthetic : 1;  /* whether item is an actual zip entry or was
                                    generated as part of a real entry's path,
                                    e.g. foo/ in a zip containing only foo/a.txt
                                    and no foo/ entry is synthetic */
#if defined(XP_UNIX) || defined(XP_BEOS)
  PRPackedBool isSymlink : 1;
#endif

  char        name[1]; // actually, bigger than 1
};

class nsZipHandle;
class nsSeekableZipHandle;
/** 
 * nsZipArchive -- a class for reading the PKZIP file format.
 *
 */
class nsZipArchive 
{
  friend class nsZipFind;

public:
  /** constructing does not open the archive. See OpenArchive() */
  nsZipArchive();

  /** destructing the object closes the archive */
  ~nsZipArchive();

  /** 
   * OpenArchive 
   * 
   * It's an error to call this more than once on the same nsZipArchive
   * object. If we were allowed to use exceptions this would have been 
   * part of the constructor 
   *
   * @param   fd            File descriptor of file to open
   * @return  status code
   */
  nsresult OpenArchive(PRFileDesc* fd);

  /**
   * Test the integrity of items in this archive by running
   * a CRC check after extracting each item into a memory 
   * buffer.  If an entry name is supplied only the 
   * specified item is tested.  Else, if null is supplied
   * then all the items in the archive are tested.
   *
   * @return  status code       
   */
  nsresult Test(const char *aEntryName);

  /**
   * Closes an open archive.
   */
  nsresult CloseArchive();

  /** 
   * GetItem
   * @param   aEntryName Name of file in the archive
   * @return  pointer to nsZipItem
   */  
  nsZipItem* GetItem(const char * aEntryName);
  
  /** 
   * ExtractFile
   *
   * @param   zipEntry   Name of file in archive to extract
   * @param   outFD      Filedescriptor to write contents to
   * @param   outname    Name of file to write to
   * @return  status code
   */
  nsresult ExtractFile(nsZipItem * zipEntry, const char *outname, PRFileDesc * outFD);

  /**
   * FindInit
   *
   * Initializes a search for files in the archive. FindNext() returns
   * the actual matches. The nsZipFind must be deleted when you're done
   *
   * @param   aPattern    a string or RegExp pattern to search for
   *                      (may be NULL to find all files in archive)
   * @param   aFind       a pointer to a pointer to a structure used
   *                      in FindNext.  In the case of an error this
   *                      will be set to NULL.
   * @return  status code
   */
  PRInt32 FindInit(const char * aPattern, nsZipFind** aFind);

  /* Gets an undependent handle to the jar
   * Also ensures that aItem is fully filled
   */
  nsZipHandle* GetFD(nsZipItem* aItem);

private:
  //--- private members ---

  nsZipItem*    mFiles[ZIP_TABSIZE];
  PLArenaPool   mArena;

  /**
   * Fills in nsZipItem fields that were not filled in by BuildFileList
   * @param   aItem       Pointer to nsZipItem
   * returns true if the item was filled in successfully
   */
  bool MaybeReadItem(nsZipItem* aItem);

  // Whether we synthesized the directory entries
  PRPackedBool  mBuiltSynthetics;

  // file handle
  nsRefPtr<nsZipHandle> mFd;
  //--- private methods ---
  
  nsZipArchive& operator=(const nsZipArchive& rhs); // prevent assignments
  nsZipArchive(const nsZipArchive& rhs);            // prevent copies

  nsZipItem*        CreateZipItem(PRUint16 namelen);
  nsresult          BuildFileList();
  nsresult          BuildSynthetics();

  nsresult  CopyItemToDisk(PRUint32 size, PRUint32 crc, nsSeekableZipHandle &fd, PRFileDesc* outFD);
  nsresult  InflateItem(const nsZipItem* aItem, nsSeekableZipHandle &fd, PRFileDesc* outFD);
};

class nsZipHandle {
friend class nsZipArchive;
friend class nsSeekableZipHandle;
public:
  static nsresult Init(PRFileDesc *fd, nsZipHandle **ret NS_OUTPARAM);

  /**
   * Reads data at a certain point
   * @param aPosition seek ofset
   * @param aBuffer buffer
   * @param aCount number of bytes to read */
  PRInt32 Read(PRUint32 aPosition, void *aBuffer, PRUint32 aCount);

  NS_METHOD_(nsrefcnt) AddRef(void);
  NS_METHOD_(nsrefcnt) Release(void);

protected:
  PRFileDesc *mFd; // OS file-descriptor
  PRUint8 *mFileData; // pointer to mmaped file
  PRUint32 mLen; // length of file and memory mapped area

private:
  nsZipHandle();
  ~nsZipHandle();

  PRFileMap *mMap; // nspr datastructure for mmap
  nsrefcnt mRefCnt; // ref count
};


/** nsSeekableZipHandle acts as a container for nsZipHandle,
    emulates sequential file io */
class nsSeekableZipHandle {
  //   stick nsZipItem in here
public:
  nsSeekableZipHandle()
    : mOffset(0)
    , mRemaining(0)
  {
  }

  /** Initializes nsSeekableZipHandle with
   * @param aOffset byte offset of the file to start reading at
   * @param length of this descriptor
   */
  bool Open(nsZipHandle *aHandle, PRUint32 aOffset, PRUint32 aLength) {
    NS_ABORT_IF_FALSE (aHandle, "Argument must not be NULL");
    if (aOffset > aHandle->mLen)
      return false;
    mFd = aHandle;
    mOffset = aOffset;
    mRemaining = aLength;
    return true;
  }

  /** Releases the file handle. It is safe to call multiple times. */
  void Close()
  {
    mFd = NULL;
  }

  /**
   * Reads data at a certain point
   * @param aBuffer input buffer
   * @param aCount number of bytes to read */
  PRInt32 Read(void *aBuffer, PRUint32 aCount)
  {
    if (!mFd.get())
      return -1;
    aCount = PR_MIN(mRemaining, aCount);
    PRInt32 ret = mFd->Read(mOffset, aBuffer, aCount);
    if (ret > 0) {
      mOffset += ret;
      mRemaining -= ret;
    }
    return ret;
  }

private:
  nsRefPtr<nsZipHandle> mFd; // file handle
  PRUint32 mOffset; // current reading offset
  PRUint32 mRemaining; // bytes remaining
};


/** 
 * nsZipFind 
 *
 * a helper class for nsZipArchive, representing a search
 */
class nsZipFind
{
public:

  nsZipFind(nsZipArchive* aZip, char* aPattern, PRBool regExp);
  ~nsZipFind();

  nsresult      FindNext(const char ** aResult);

private:
  nsZipArchive* mArchive;
  char*         mPattern;
  nsZipItem*    mItem;
  PRUint16      mSlot;
  PRPackedBool  mRegExp;

  //-- prevent copies and assignments
  nsZipFind& operator=(const nsZipFind& rhs);
  nsZipFind(const nsZipFind& rhs);
};

nsresult gZlibInit(z_stream *zs);

#endif /* nsZipArchive_h_ */
