// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

(() => {
  /**
   * Tests that the directory tree can be vertically scrolled.
   */
  testcase.directoryTreeVerticalScroll = async () => {
    const folders = [ENTRIES.photos];

    // Add enough test entries to overflow the #directory-tree container.
    for (let i = 0; i < 30; i++) {
      folders.push(new TestEntryInfo({
        type: EntryType.DIRECTORY,
        targetPath: '' + i,
        lastModifiedTime: 'Jan 1, 1980, 11:59 PM',
        nameText: '' + i,
        sizeText: '--',
        typeText: 'Folder'
      }));
    }

    // Open FilesApp on Downloads and expand the tree view of Downloads.
    const appId = await setupAndWaitUntilReady(RootPath.DOWNLOADS, folders, []);
    await expandRoot(appId, TREEITEM_DOWNLOADS);

    // Verify the directory tree is not vertically scrolled.
    const directoryTree = '#directory-tree';
    const original = await remoteCall.waitForElementStyles(
        appId, directoryTree, ['scrollTop']);
    chrome.test.assertTrue(original.scrollTop === 0);

    // Scroll the directory tree down (vertical scroll).
    await remoteCall.callRemoteTestUtil(
        'setScrollTop', appId, [directoryTree, 100]);

    // Check: the directory tree should be vertically scrolled.
    const scrolled = await remoteCall.waitForElementStyles(
        appId, directoryTree, ['scrollTop']);
    const scrolledDown = scrolled.scrollTop === 100;
    chrome.test.assertTrue(scrolledDown, 'Tree should scroll down');
  };

  /**
   * Tests that the directory tree does not horizontally scroll.
   */
  testcase.directoryTreeHorizontalScroll = async () => {
    // Open FilesApp on Downloads and expand the tree view of Downloads.
    const appId = await setupAndWaitUntilReady(
        RootPath.DOWNLOADS, BASIC_LOCAL_ENTRY_SET, []);
    await expandRoot(appId, TREEITEM_DOWNLOADS);

    // Verify the directory tree is not horizontally scrolled.
    const directoryTree = '#directory-tree';
    const original = await remoteCall.waitForElementStyles(
        appId, directoryTree, ['scrollLeft']);
    chrome.test.assertTrue(original.scrollLeft === 0);

    // Shrink the tree to 50px. TODO(files-ng): consider using 150px?
    const navigationList = '.dialog-navigation-list';
    await remoteCall.callRemoteTestUtil(
        'setElementStyles', appId, [navigationList, {width: '50px'}]);

    // Scroll the directory tree left (horizontal scroll).
    await remoteCall.callRemoteTestUtil(
        'setScrollLeft', appId, [directoryTree, 100]);

    // Check: the directory tree should not be horizontally scrolled.
    const scrolled = await remoteCall.waitForElementStyles(
        appId, directoryTree, ['scrollLeft']);
    const noScrollLeft = scrolled.scrollLeft === 0;
    chrome.test.assertTrue(noScrollLeft, 'Tree should not scroll left');
  };

  /**
   * Tests that the directory tree does not horizontally scroll when expanding
   * nested folder items.
   */
  testcase.directoryTreeExpandHorizontalScroll = async () => {
    /**
     * Creates a folder test entry from a folder |path|.
     * @param {string} path The folder path.
     * @return {!TestEntryInfo}
     */
    function createFolderTestEntry(path) {
      const name = path.split('/').pop();
      return new TestEntryInfo({
        targetPath: path,
        nameText: name,
        type: EntryType.DIRECTORY,
        lastModifiedTime: 'Jan 1, 1980, 11:59 PM',
        sizeText: '--',
        typeText: 'Folder',
      });
    }

    // Build an array of nested folder test entries.
    const nestedFolderTestEntries = [];
    for (let path = 'nested-folder1', i = 0; i < 6; ++i) {
      nestedFolderTestEntries.push(createFolderTestEntry(path));
      path += `/nested-folder${i + 1}`;
    }

    // Open FilesApp on Downloads containing the folder test entries.
    const appId = await setupAndWaitUntilReady(
        RootPath.DOWNLOADS, nestedFolderTestEntries, []);

    // Verify the directory tree is not horizontally scrolled.
    const directoryTree = '#directory-tree';
    const original = await remoteCall.waitForElementStyles(
        appId, directoryTree, ['scrollLeft']);
    chrome.test.assertTrue(original.scrollLeft === 0);

    // Shrink the tree to 150px, enough to hide the deep folder names.
    const navigationList = '.dialog-navigation-list';
    await remoteCall.callRemoteTestUtil(
        'setElementStyles', appId, [navigationList, {width: '150px'}]);

    // Expand the tree Downloads > nested-folder1 > nested-folder2 ...
    const lastFolderPath = nestedFolderTestEntries.pop().targetPath;
    await remoteCall.navigateWithDirectoryTree(
        appId, '/Downloads/' + lastFolderPath, 'My files');

    // Check: the directory tree should be showing the last test entry.
    await remoteCall.waitForElement(
        appId, '.tree-item[entry-label="nested-folder5"]:not([hidden])');

    // Ensure the directory tree scroll event handling is complete.
    chrome.test.assertTrue(await remoteCall.callRemoteTestUtil(
        'requestAnimationFrame', appId, []));

    // Check: the directory tree should not be horizontally scrolled.
    const scrolled = await remoteCall.waitForElementStyles(
        appId, directoryTree, ['scrollLeft']);
    const noScrollLeft = scrolled.scrollLeft === 0;
    chrome.test.assertTrue(noScrollLeft, 'Tree should not scroll left');
  };
})();
