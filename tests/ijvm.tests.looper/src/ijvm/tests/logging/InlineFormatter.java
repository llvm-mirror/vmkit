package ijvm.tests.logging;

import java.util.*;
import java.util.logging.*;

public class InlineFormatter
	extends java.util.logging.Formatter
{
	public static Logger createLogger(String className)
	{
		Logger log = Logger.getAnonymousLogger();
		
		Handler logConsoleHandler = new ConsoleHandler();
		logConsoleHandler.setFormatter(new InlineFormatter());
		log.addHandler(logConsoleHandler);
		
		log.setUseParentHandlers(false);
		log.setLevel(Level.INFO);
		return log;
	}
	
	public String format(LogRecord record)
	{
		Calendar cal = GregorianCalendar.getInstance();
		cal.setTime(new Date(record.getMillis()));
		String logTime = cal.get(Calendar.HOUR_OF_DAY) + ":" +
			cal.get(Calendar.MINUTE) + ":" +
			cal.get(Calendar.SECOND) + "." +
			cal.get(Calendar.MILLISECOND);
		
		StringBuffer str = new StringBuffer(256);
		str.append(record.getLevel().getName() +
			"[" + logTime +
			" @ " + Integer.toHexString(record.getThreadID()) + " " +
			record.getSourceClassName() + "." + record.getSourceMethodName() + "] " +
			record.getMessage() + "\n"
			);
		
		return str.toString();
	}
}
