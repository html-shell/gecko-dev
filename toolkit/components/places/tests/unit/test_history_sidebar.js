/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et: */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Ondrej Brablc <ondrej@allpeers.com>
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

// Get history service
var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bh = hs.QueryInterface(Ci.nsIBrowserHistory);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var ps = Cc["@mozilla.org/preferences-service;1"].
         getService(Ci.nsIPrefBranch);

/**
 * Adds a test URI visit to the database, and checks for a valid place ID.
 *
 * @param aURI
 *        The URI to add a visit for.
 * @param aReferrer
 *        The referring URI for the given URI.  This can be null.
 * @returns the place id for aURI.
 */
function add_normalized_visit(aURI, aTime, aDayOffset) {
  var dateObj = new Date(aTime);
  // Normalize to midnight
  dateObj.setHours(0);
  dateObj.setMinutes(0);
  dateObj.setSeconds(0);
  dateObj.setMilliseconds(0);
  // Days where DST changes should be taken in count.
  var previousDateObj = new Date(dateObj.getTime() + aDayOffset * 86400000);
  var DSTCorrection = (dateObj.getTimezoneOffset() - previousDateObj.getTimezoneOffset()) * 60 * 1000;
  // Substract aDayOffset
  var PRTimeWithOffset = (previousDateObj.getTime() - DSTCorrection) * 1000;
  print("Adding visit to " + aURI.spec + " at " + new Date(PRTimeWithOffset/1000));
  var visitId = hs.addVisit(aURI,
                            PRTimeWithOffset,
                            null,
                            hs.TRANSITION_TYPED, // user typed in URL bar
                            false, // not redirect
                            0);
  do_check_true(visitId > 0);
  return visitId;
}

var nowObj = new Date();
// This test relies on en-US locale
// Offset is number of days
var containers = [
  { label: "Today",               offset: 0                     },
  { label: "Yesterday",           offset: -1                    },
  { label: "Last 7 days",         offset: -3                    },
  { label: "This month",          offset: -8                    },
  { label: "",                    offset: -nowObj.getDate()-1   },
  { label: "Older than 6 months", offset: -nowObj.getDate()-186 },
];

/**
 * Fills history and checks containers' labels.
 */
function fill_history() {
  print("\n\n*** TEST Fill History\n");
  // We can't use "now" because our hardcoded offsets would be invalid for some
  // date.  So we hardcode a date.
  for (var i = 0; i < containers.length; i++) {
    var container = containers[i];
    var testURI = uri("http://mirror"+i+".mozilla.com/b");
    add_normalized_visit(testURI, nowObj.getTime(), container.offset);
    var testURI = uri("http://mirror"+i+".mozilla.com/a");
    add_normalized_visit(testURI, nowObj.getTime(), container.offset);
    var testURI = uri("http://mirror"+i+".google.com/b");
    add_normalized_visit(testURI, nowObj.getTime(), container.offset);
    var testURI = uri("http://mirror"+i+".google.com/a");
    add_normalized_visit(testURI, nowObj.getTime(), container.offset);
  }

  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_SITE_QUERY;
  var query = hs.getNewQuery();

  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  var cc = root.childCount;
  print("Found containers:");
  for (var i = 0; i < cc; i++) {
    var container = containers[i];
    var node = root.getChild(i);
    print(node.title);
    if (container.label)
      do_check_eq(node.title, container.label);
  }
  do_check_eq(cc, containers.length);
  root.containerOpen = false;
}

/**
 * Queries history grouped by date and site, checking containers' labels and
 * children.
 */
function test_RESULTS_AS_DATE_SITE_QUERY() {
  print("\n\n*** TEST RESULTS_AS_DATE_SITE_QUERY\n");
  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_SITE_QUERY;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  // Check one of the days
  var dayNode = root.getChild(0)
                    .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayNode.containerOpen = true;
  do_check_eq(dayNode.childCount, 2);

  // Items should be sorted by host
  var site1 = dayNode.getChild(0)
                     .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(site1.title, "mirror0.google.com");

  var site2 = dayNode.getChild(1)
                     .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(site2.title, "mirror0.mozilla.com");

  site1.containerOpen = true;
  do_check_eq(site1.childCount, 2);

  // Inside of host sites are sorted by title
  var site1visit = site1.getChild(0);
  do_check_eq(site1visit.uri, "http://mirror0.google.com/a");

  site1.containerOpen = false;
  dayNode.containerOpen = false;
  root.containerOpen = false;
}

/**
 * Queries history grouped by date, checking containers' labels and children.
 */
function test_RESULTS_AS_DATE_QUERY() {
  print("\n\n*** TEST RESULTS_AS_DATE_QUERY\n");
  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_QUERY;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  var cc = root.childCount;
  do_check_eq(cc, containers.length);
  print("Found containers:");
  for (var i = 0; i < cc; i++) {
    var container = containers[i];
    var node = root.getChild(i);
    print(node.title);
    if (container.label)
      do_check_eq(node.title, container.label);
  }

  // Check one of the days
  var dayNode = root.getChild(0)
                    .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayNode.containerOpen = true;
  do_check_eq(dayNode.childCount, 4);

  // Items should be sorted by title
  var visit1 = dayNode.getChild(0);
  do_check_eq(visit1.uri, "http://mirror0.google.com/a");

  var visit2 = dayNode.getChild(3);
  do_check_eq(visit2.uri, "http://mirror0.mozilla.com/b");

  dayNode.containerOpen = false;
  root.containerOpen = false;
}

/**
 * Queries history grouped by site, checking containers' labels and children.
 */
function test_RESULTS_AS_SITE_QUERY() {
  print("\n\n*** TEST RESULTS_AS_SITE_QUERY\n");
  // add a bookmark with a domain not in the set of visits in the db
  bs.insertBookmark(bs.toolbarFolder, uri("http://foobar"),
                    bs.DEFAULT_INDEX, "");

  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_SITE_QUERY;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, containers.length * 2);

/* Expected results:
    "mirror0.google.com",
    "mirror0.mozilla.com",
    "mirror1.google.com",
    "mirror1.mozilla.com",
    "mirror2.google.com",
    "mirror2.mozilla.com",
    "mirror3.google.com",  <== We check for this site (index 6)
    "mirror3.mozilla.com",
    "mirror4.google.com",
    "mirror4.mozilla.com",
    "mirror5.google.com",
    "mirror5.mozilla.com",
    ...
*/

  // Items should be sorted by host
  var siteNode = root.getChild(6)
                     .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(siteNode.title, "mirror3.google.com");

  siteNode.containerOpen = true;
  do_check_eq(siteNode.childCount, 2);

  // Inside of host sites are sorted by title
  var visitNode = siteNode.getChild(0);
  do_check_eq(visitNode.uri, "http://mirror3.google.com/a");

  siteNode.containerOpen = false;
  root.containerOpen = false;
}

// main
function run_test() {
  // Increase history limit to 1 year
  ps.setIntPref("browser.history_expire_days", 365);

  // Cleanup.
  bh.removeAllPages();

  fill_history();
  test_RESULTS_AS_DATE_SITE_QUERY();
  test_RESULTS_AS_DATE_QUERY();
  test_RESULTS_AS_SITE_QUERY();

  // The remaining views are
  //   RESULTS_AS_URI + SORT_BY_VISITCOUNT_DESCENDING 
  //   ->  test_399266.js
  //   RESULTS_AS_URI + SORT_BY_DATE_DESCENDING
  //   ->  test_385397.js
}
