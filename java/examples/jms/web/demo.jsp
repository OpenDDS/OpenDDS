<%--
  Created by IntelliJ IDEA.
  User: gaow
  Date: Nov 18, 2008
  Time: 3:23:34 PM
  To change this template use File | Settings | File Templates.
--%>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<html>
<head>
    <title>OpenDDS JMS Provider Demos</title>
</head>
<body>
<h2>OpenDDS JMS Provider Demo</h2>

<h3>Result</h3>

<p><%= request.getAttribute("toDisplay")%></p>

<h3>Actions</h3>

<form name="Connection" method="GET" action="actions/createConnection&action=send">
    <input type="submit" value="Send"/>
</form>

<br/>

<form name="Connection" method="GET" action="actions/createConnection&action=receive">
    <input type="submit" value="Receive"/>
</form>

</body>
</html>