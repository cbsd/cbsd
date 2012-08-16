/*
 * 
 * 	Part of NAS4Free (http://www.nas4free.org).
 * 	Copyright (C) 2012 by NAS4Free Team <info@nas4free.org>.
 * 	All rights reserved.
 * 
 * 	Portions of freenas (http://www.freenas.org).
 * 	Copyright (C) 2005-2011 by Olivier Cochard <olivier@freenas.org>.
 * 	All rights reserved.
 * 
 * 
 * 	Copyright (C) 2004 Baron Schwartz <baron at sequent dot org> 
 * 	http://www.xaprb.com/blog/2005/09/29/javascript-date-chooser
 *
 * 	Modified by Volker Theile (votdev@gmx.de) 04/2008.
 *
 * 	This program is free software; you can redistribute it and/or modify it
 * 	under the terms of the GNU Lesser General Public License as published by the
 * 	Free Software Foundation, version 2.1.
 *
 * 	This program is distributed in the hope that it will be useful, but WITHOUT
 * 	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * 	FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * 	details.
 *
 * 	$Revision: 1.1 $
 */

Date.parseFunctions = {count:0};
Date.parseRegexes = [];
Date.formatFunctions = {count:0};

Date.prototype.dateFormat = function(format) {
    if (Date.formatFunctions[format] == null) {
        Date.createNewFormat(format);
    }
    var func = Date.formatFunctions[format];
    return this[func]();
}

Date.createNewFormat = function(format) {
    var funcName = "format" + Date.formatFunctions.count++;
    Date.formatFunctions[format] = funcName;
    var code = "Date.prototype." + funcName + " = function(){return ";
    var special = false;
    var ch = '';
    for (var i = 0; i < format.length; ++i) {
        ch = format.charAt(i);
        if (!special && ch == "\\") {
            special = true;
        }
        else if (special) {
            special = false;
            code += "'" + String.escape(ch) + "' + ";
        }
        else {
            code += Date.getFormatCode(ch);
        }
    }
    eval(code.substring(0, code.length - 3) + ";}");
}

Date.getFormatCode = function(character) {
    switch (character) {
    case "d":
        return "String.leftPad(this.getDate(), 2, '0') + ";
    case "D":
        return "Date.dayNames[this.getDay()].substring(0, 3) + ";
    case "j":
        return "this.getDate() + ";
    case "l":
        return "Date.dayNames[this.getDay()] + ";
    case "S":
        return "this.getSuffix() + ";
    case "w":
        return "this.getDay() + ";
    case "z":
        return "this.getDayOfYear() + ";
    case "W":
        return "this.getWeekOfYear() + ";
    case "F":
        return "Date.monthNames[this.getMonth()] + ";
    case "m":
        return "String.leftPad(this.getMonth() + 1, 2, '0') + ";
    case "M":
        return "Date.monthNames[this.getMonth()].substring(0, 3) + ";
    case "n":
        return "(this.getMonth() + 1) + ";
    case "t":
        return "this.getDaysInMonth() + ";
    case "L":
        return "(this.isLeapYear() ? 1 : 0) + ";
    case "Y":
        return "this.getFullYear() + ";
    case "y":
        return "('' + this.getFullYear()).substring(2, 4) + ";
    case "a":
        return "(this.getHours() < 12 ? 'am' : 'pm') + ";
    case "A":
        return "(this.getHours() < 12 ? 'AM' : 'PM') + ";
    case "g":
        return "((this.getHours() %12) ? this.getHours() % 12 : 12) + ";
    case "G":
        return "this.getHours() + ";
    case "h":
        return "String.leftPad((this.getHours() %12) ? this.getHours() % 12 : 12, 2, '0') + ";
    case "H":
        return "String.leftPad(this.getHours(), 2, '0') + ";
    case "i":
        return "String.leftPad(this.getMinutes(), 2, '0') + ";
    case "s":
        return "String.leftPad(this.getSeconds(), 2, '0') + ";
    case "O":
        return "this.getGMTOffset() + ";
    case "T":
        return "this.getTimezone() + ";
    case "Z":
        return "(this.getTimezoneOffset() * -60) + ";
    default:
        return "'" + String.escape(character) + "' + ";
    }
}

Date.parseDate = function(input, format) {
    if (Date.parseFunctions[format] == null) {
        Date.createParser(format);
    }
    var func = Date.parseFunctions[format];
    return Date[func](input);
}

Date.createParser = function(format) {
    var funcName = "parse" + Date.parseFunctions.count++;
    var regexNum = Date.parseRegexes.length;
    var currentGroup = 1;
    Date.parseFunctions[format] = funcName;

    var code = "Date." + funcName + " = function(input){\n"
        + "var y = -1, m = -1, d = -1, h = -1, i = -1, s = -1;\n"
        + "var d = new Date();\n"
        + "y = d.getFullYear();\n"
        + "m = d.getMonth();\n"
        + "d = d.getDate();\n"
        + "var results = input.match(Date.parseRegexes[" + regexNum + "]);\n"
        + "if (results && results.length > 0) {"
    var regex = "";

    var special = false;
    var ch = '';
    for (var i = 0; i < format.length; ++i) {
        ch = format.charAt(i);
        if (!special && ch == "\\") {
            special = true;
        }
        else if (special) {
            special = false;
            regex += String.escape(ch);
        }
        else {
            obj = Date.formatCodeToRegex(ch, currentGroup);
            currentGroup += obj.g;
            regex += obj.s;
            if (obj.g && obj.c) {
                code += obj.c;
            }
        }
    }

    code += "if (y > 0 && m >= 0 && d > 0 && h >= 0 && i >= 0 && s >= 0)\n"
        + "{return new Date(y, m, d, h, i, s);}\n"
        + "else if (y > 0 && m >= 0 && d > 0 && h >= 0 && i >= 0)\n"
        + "{return new Date(y, m, d, h, i);}\n"
        + "else if (y > 0 && m >= 0 && d > 0 && h >= 0)\n"
        + "{return new Date(y, m, d, h);}\n"
        + "else if (y > 0 && m >= 0 && d > 0)\n"
        + "{return new Date(y, m, d);}\n"
        + "else if (y > 0 && m >= 0)\n"
        + "{return new Date(y, m);}\n"
        + "else if (y > 0)\n"
        + "{return new Date(y);}\n"
        + "}return null;}";

    Date.parseRegexes[regexNum] = new RegExp("^" + regex + "$");
    eval(code);
}

Date.formatCodeToRegex = function(character, currentGroup) {
    switch (character) {
    case "D":
        return {g:0,
        c:null,
        s:"(?:Sun|Mon|Tue|Wed|Thu|Fri|Sat)"};
    case "j":
    case "d":
        return {g:1,
            c:"d = parseInt(results[" + currentGroup + "], 10);\n",
            s:"(\\d{1,2})"};
    case "l":
        return {g:0,
            c:null,
            s:"(?:" + Date.dayNames.join("|") + ")"};
    case "S":
        return {g:0,
            c:null,
            s:"(?:st|nd|rd|th)"};
    case "w":
        return {g:0,
            c:null,
            s:"\\d"};
    case "z":
        return {g:0,
            c:null,
            s:"(?:\\d{1,3})"};
    case "W":
        return {g:0,
            c:null,
            s:"(?:\\d{2})"};
    case "F":
        return {g:1,
            c:"m = parseInt(Date.monthNumbers[results[" + currentGroup + "].substring(0, 3)], 10);\n",
            s:"(" + Date.monthNames.join("|") + ")"};
    case "M":
        return {g:1,
            c:"m = parseInt(Date.monthNumbers[results[" + currentGroup + "]], 10);\n",
            s:"(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)"};
    case "n":
    case "m":
        return {g:1,
            c:"m = parseInt(results[" + currentGroup + "], 10) - 1;\n",
            s:"(\\d{1,2})"};
    case "t":
        return {g:0,
            c:null,
            s:"\\d{1,2}"};
    case "L":
        return {g:0,
            c:null,
            s:"(?:1|0)"};
    case "Y":
        return {g:1,
            c:"y = parseInt(results[" + currentGroup + "], 10);\n",
            s:"(\\d{4})"};
    case "y":
        return {g:1,
            c:"var ty = parseInt(results[" + currentGroup + "], 10);\n"
                + "y = ty > Date.y2kYear ? 1900 + ty : 2000 + ty;\n",
            s:"(\\d{1,2})"};
    case "a":
        return {g:1,
            c:"if (results[" + currentGroup + "] == 'am') {\n"
                + "if (h == 12) { h = 0; }\n"
                + "} else { if (h < 12) { h += 12; }}",
            s:"(am|pm)"};
    case "A":
        return {g:1,
            c:"if (results[" + currentGroup + "] == 'AM') {\n"
                + "if (h == 12) { h = 0; }\n"
                + "} else { if (h < 12) { h += 12; }}",
            s:"(AM|PM)"};
    case "g":
    case "G":
    case "h":
    case "H":
        return {g:1,
            c:"h = parseInt(results[" + currentGroup + "], 10);\n",
            s:"(\\d{1,2})"};
    case "i":
        return {g:1,
            c:"i = parseInt(results[" + currentGroup + "], 10);\n",
            s:"(\\d{2})"};
    case "s":
        return {g:1,
            c:"s = parseInt(results[" + currentGroup + "], 10);\n",
            s:"(\\d{2})"};
    case "O":
        return {g:0,
            c:null,
            s:"[+-]\\d{4}"};
    case "T":
        return {g:0,
            c:null,
            s:"[A-Z]{3}"};
    case "Z":
        return {g:0,
            c:null,
            s:"[+-]\\d{1,5}"};
    default:
        return {g:0,
            c:null,
            s:String.escape(character)};
    }
}

Date.prototype.getTimezone = function() {
    return this.toString().replace(
        /^.*? ([A-Z]{3}) [0-9]{4}.*$/, "$1").replace(
        /^.*?\(([A-Z])[a-z]+ ([A-Z])[a-z]+ ([A-Z])[a-z]+\)$/, "$1$2$3");
}

Date.prototype.getGMTOffset = function() {
    return (this.getTimezoneOffset() > 0 ? "-" : "+")
        + String.leftPad(Math.floor(this.getTimezoneOffset() / 60), 2, "0")
        + String.leftPad(this.getTimezoneOffset() % 60, 2, "0");
}

Date.prototype.getDayOfYear = function() {
    var num = 0;
    Date.daysInMonth[1] = this.isLeapYear() ? 29 : 28;
    for (var i = 0; i < this.getMonth(); ++i) {
        num += Date.daysInMonth[i];
    }
    return num + this.getDate() - 1;
}

Date.prototype.getWeekOfYear = function() {
    // Skip to Thursday of this week
    var now = this.getDayOfYear() + (4 - this.getDay());
    // Find the first Thursday of the year
    var jan1 = new Date(this.getFullYear(), 0, 1);
    var then = (7 - jan1.getDay() + 4);
    document.write(then);
    return String.leftPad(((now - then) / 7) + 1, 2, "0");
}

Date.prototype.isLeapYear = function() {
    var year = this.getFullYear();
    return ((year & 3) == 0 && (year % 100 || (year % 400 == 0 && year)));
}

Date.prototype.getFirstDayOfMonth = function() {
    var day = (this.getDay() - (this.getDate() - 1)) % 7;
    return (day < 0) ? (day + 7) : day;
}

Date.prototype.getLastDayOfMonth = function() {
    var day = (this.getDay() + (Date.daysInMonth[this.getMonth()] - this.getDate())) % 7;
    return (day < 0) ? (day + 7) : day;
}

Date.prototype.getDaysInMonth = function() {
    Date.daysInMonth[1] = this.isLeapYear() ? 29 : 28;
    return Date.daysInMonth[this.getMonth()];
}

Date.prototype.getSuffix = function() {
    switch (this.getDate()) {
        case 1:
        case 21:
        case 31:
            return "st";
        case 2:
        case 22:
            return "nd";
        case 3:
        case 23:
            return "rd";
        default:
            return "th";
    }
}

String.escape = function(string) {
    return string.replace(/('|\\)/g, "\\$1");
}

String.leftPad = function (val, size, ch) {
    var result = new String(val);
    if (ch == null) {
        ch = " ";
    }
    while (result.length < size) {
        result = ch + result;
    }
    return result;
}

Date.daysInMonth = [31,28,31,30,31,30,31,31,30,31,30,31];
Date.monthNames =
   ["January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"];
Date.dayNames =
   ["Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"];
Date.y2kYear = 50;
Date.monthNumbers = {
    Jan:0,
    Feb:1,
    Mar:2,
    Apr:3,
    May:4,
    Jun:5,
    Jul:6,
    Aug:7,
    Sep:8,
    Oct:9,
    Nov:10,
    Dec:11};
Date.patterns = {
		Default:"m/d/Y H:i",
    ISO8601LongPattern:"Y-m-d H:i:s",
    ISO8601ShortPattern:"Y-m-d",
    ShortDatePattern: "n/j/Y",
    LongDatePattern: "l, F d, Y",
    FullDateTimePattern: "l, F d, Y g:i:s A",
    MonthDayPattern: "F d",
    ShortTimePattern: "g:i A",
    LongTimePattern: "g:i:s A",
    SortableDateTimePattern: "Y-m-d\\TH:i:s",
    UniversalSortableDateTimePattern: "Y-m-d H:i:sO",
    YearMonthPattern: "F, Y"};

// Shows or hides the date chooser on the page
function showChooser(obj, inputId, divId, start, end, format, isTimeChooser) {
    if (document.getElementById) {
        var input = document.getElementById(inputId);
        var div = document.getElementById(divId);
        if (input !== undefined && div !== undefined) {
            if (input.DateChooser === undefined) {
                input.DateChooser = new DateChooser(input, div, start, end, format, isTimeChooser);
            }
            input.DateChooser.setDate(Date.parseDate(input.value, format));
            if (input.DateChooser.isVisible()) {
                input.DateChooser.hide();
            }
            else {
                input.DateChooser.show();
            }
        }
    }
}

// Sets a date on the object attached to 'inputId'
function dateChooserSetDate(inputId, value) {
    var input = document.getElementById(inputId);
    if (input !== undefined && input.DateChooser !== undefined) {
        input.DateChooser.setDate(Date.parseDate(value, input.DateChooser._format));
        if (input.DateChooser.isTimeChooser()) {
            var theForm = input.form;
            var prefix = input.DateChooser._prefix;
            input.DateChooser.setTime(
                parseInt(theForm.elements[prefix + 'hour'].options[
                    theForm.elements[prefix + 'hour'].selectedIndex].value)
                    + parseInt(theForm.elements[prefix + 'ampm'].options[
                    theForm.elements[prefix + 'ampm'].selectedIndex].value),
                parseInt(theForm.elements[prefix + 'min'].options[
                    theForm.elements[prefix + 'min'].selectedIndex].value));
        }
        input.value = input.DateChooser.getValue();
        input.DateChooser.hide();
    }
}

// The callback function for when someone changes the pulldown menus on the date
// chooser
function dateChooserDateChange(theForm, prefix) {
    var input = document.getElementById(
        theForm.elements[prefix + 'inputId'].value);
    var newDate = new Date(
        theForm.elements[prefix + 'year'].options[
            theForm.elements[prefix + 'year'].selectedIndex].value,
        theForm.elements[prefix + 'month'].options[
            theForm.elements[prefix + 'month'].selectedIndex].value,
        1);
    // Try to preserve the day of month (watch out for months with 31 days)
    newDate.setDate(Math.max(1, Math.min(newDate.getDaysInMonth(),
                    input.DateChooser._date.getDate())));
    input.DateChooser.setDate(newDate);
    if (input.DateChooser.isTimeChooser()) {
        input.DateChooser.setTime(
            parseInt(theForm.elements[prefix + 'hour'].options[
                theForm.elements[prefix + 'hour'].selectedIndex].value)
                + parseInt(theForm.elements[prefix + 'ampm'].options[
                theForm.elements[prefix + 'ampm'].selectedIndex].value),
            parseInt(theForm.elements[prefix + 'min'].options[
                theForm.elements[prefix + 'min'].selectedIndex].value));
    }
    input.DateChooser.show();
}

// Gets the absolute position on the page of the element passed in
function getAbsolutePosition(obj) {
    var result = [0, 0];
    while (obj != null) {
        result[0] += obj.offsetTop;
        result[1] += obj.offsetLeft;
        obj = obj.offsetParent;
    }
    return result;
}

// DateChooser constructor
function DateChooser(input, div, start, end, format, isTimeChooser) {
    this._input = input;
    this._div = div;
    this._start = start;
    this._end = end;
    this._format = format;
    this._date = new Date();
    this._isTimeChooser = isTimeChooser;
    // Choose a random prefix for all pulldown menus
    this._prefix = "";
    var letters = ["a", "b", "c", "d", "e", "f", "h", "i", "j", "k", "l",
        "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"];
    for (var i = 0; i < 10; ++i) {
        this._prefix += letters[Math.floor(Math.random() * letters.length)];
    }
}

// DateChooser prototype variables
DateChooser.prototype._isVisible = false;

// Returns true if the chooser is currently visible
DateChooser.prototype.isVisible = function() {
    return this._isVisible;
}

// Returns true if the chooser is to allow choosing the time as well as the date
DateChooser.prototype.isTimeChooser = function() {
    return this._isTimeChooser;
}

// Gets the value, as a formatted string, of the date attached to the chooser
DateChooser.prototype.getValue = function() {
    return this._date.dateFormat(this._format);
}

// Hides the chooser
DateChooser.prototype.hide = function() {
    this._div.style.visibility = "hidden";
    this._div.style.display = "none";
    this._div.innerHTML = "";
    this._isVisible = false;
}

// Shows the chooser on the page
DateChooser.prototype.show = function() {
    // calculate the position before making it visible
    var inputPos = getAbsolutePosition(this._input);
    this._div.style.top = (inputPos[0] + this._input.offsetHeight) + "px";
    this._div.style.left = (inputPos[1] + this._input.offsetWidth) + "px";
    this._div.innerHTML = this.createChooserHtml();
    this._div.style.display = "block";
    this._div.style.visibility = "visible";
    this._div.style.position = "absolute";
    this._isVisible = true;
}

// Sets the date to what is in the input box
DateChooser.prototype.initializeDate = function() {
    if (this._input.value != null && this._input.value != "") {
        this._date = Date.parseDate(this._input.value, this._format);
    }
    else {
        this._date = new Date();
    }
}

// Sets the date attached to the chooser
DateChooser.prototype.setDate = function(date) {
    this._date = date ? date : new Date();
}

// Sets the time portion of the date attached to the chooser
DateChooser.prototype.setTime = function(hour, minute) {
    this._date.setHours(hour);
    this._date.setMinutes(minute);
}

// Creates the HTML for the whole chooser
DateChooser.prototype.createChooserHtml = function() {
    var formHtml = "<input type=\"hidden\" name=\""
        + this._prefix + "inputId\" value=\""
        + this._input.getAttribute('id') + "\">"
        + "\r\n  <select name=\"" + this._prefix 
        + "month\" onChange=\"dateChooserDateChange(this.form, '"
        + this._prefix + "');\">";
    for (var monIndex = 0; monIndex <= 11; monIndex++) {
        formHtml += "\r\n    <option value=\"" + monIndex + "\""
            + (monIndex == this._date.getMonth() ? " selected=\"1\"" : "")
            + ">" + Date.monthNames[monIndex] + "</option>";
    }
    formHtml += "\r\n  </select>\r\n  <select name=\""
        + this._prefix + "year\" onChange=\"dateChooserDateChange(this.form, '"
        + this._prefix + "');\">";
    for (var i = this._start; i <= this._end; ++i) {
        formHtml += "\r\n    <option value=\"" + i + "\""
            + (i == this._date.getFullYear() ? " selected=\"1\"" : "")
            + ">" + i + "</option>";
    }
    formHtml += "\r\n  </select>";
    formHtml += this.createCalendarHtml();
    if (this._isTimeChooser) {
        formHtml += this.createTimeChooserHtml();
    }
    return formHtml;
}

// Creates the extra HTML needed for choosing the time
DateChooser.prototype.createTimeChooserHtml = function() {
    // Add hours
    var result = "\r\n  <select name=\"" + this._prefix + "hour\">";
    for (var i = 0; i < 12; ++i) {
        result += "\r\n    <option value=\"" + i + "\""
            + (((this._date.getHours() % 12) == i) ? " selected=\"1\">" : ">")
            + i + "</option>";
    }
    // Add extra entry for 12:00
    result += "\r\n    <option value=\"0\">12</option>";
    result += "\r\n  </select>";
    // Add minutes
    result += "\r\n  <select name=\"" + this._prefix + "min\">";
    for (var i = 0; i < 60; i += 1) {
        result += "\r\n    <option value=\"" + i + "\""
            + ((this._date.getMinutes() == i) ? " selected=\"1\">" : ">")
            + String.leftPad(i, 2, '0') + "</option>";
    }
    result += "\r\n  </select>";
    // Add AM/PM
    result += "\r\n  <select name=\"" + this._prefix + "ampm\">";
    result += "\r\n    <option value=\"0\""
        + (this._date.getHours() < 12 ? " selected=\"1\">" : ">")
        + "AM</option>";
    result += "\r\n    <option value=\"12\""
        + (this._date.getHours() >= 12 ? " selected=\"1\">" : ">")
        + "PM</option>";
    result += "\r\n  </select>";
    return result;
}

// Creates the HTML for the actual calendar part of the chooser
DateChooser.prototype.createCalendarHtml = function() {
    var result = "\r\n<table cellspacing=\"0\" class=\"dateChooser\">"
        + "\r\n  <tr><th>S</th><th>M</th><th>T</th>"
        + "<th>W</th><th>T</th><th>F</th><th>S</th></tr>\r\n  <tr>";
    // Fill up the days of the week until we get to the first day of the month
    var firstDay = this._date.getFirstDayOfMonth();
    var lastDay = this._date.getLastDayOfMonth();
    if (firstDay != 0) {
        result += "<td colspan=\"" + firstDay + "\">&nbsp;</td>";
    }
    // Fill in the days of the month
    var i = 0;
    while (i < this._date.getDaysInMonth()) {
        if (((i++ + firstDay) % 7) == 0) {
            result += "</tr>\r\n  <tr>";
        }
        var thisDay = new Date(
            this._date.getFullYear(),
            this._date.getMonth(), i);
        var js = '"dateChooserSetDate(\''
            + this._input.getAttribute('id') + "', '"
            + thisDay.dateFormat(this._format) + '\');"'
        result += "\r\n    <td class=\"dateChooserActive"
            // If the date is the currently chosen date, highlight it
            + (i == this._date.getDate() ? " dateChooserActiveToday" : "")
            + "\" onClick=" + js + ">" + i + "</td>";
    }
    // Fill in any days after the end of the month
    if (lastDay != 6) {
        result += "<td colspan=\"" + (6 - lastDay) + "\">&nbsp;</td>";
    }
    return result + "\r\n  </tr>\r\n</table><!--[if lte IE 6.5]><iframe></iframe><![endif]-->";
}
