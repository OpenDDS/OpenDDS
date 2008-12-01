/*
 * $Id$
 */

package org.opendds.jms.management;

import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

import org.opendds.jms.common.util.JndiHelper;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.common.util.PropertiesHelper;
import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Constructor;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.KeyProperty;
import org.opendds.jms.management.annotation.Operation;
import org.opendds.jms.persistence.PersistenceManager;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
@Description("OpenDDS PersistenceManager MBean")
public class PersistenceManagerService extends DynamicMBeanSupport implements ServiceMBean {
    private Logger logger;

    private String service;
    private boolean started;
    private String jndiName;
    private PersistenceManager instance;

    private JndiHelper helper = new JndiHelper();

    @Constructor
    public PersistenceManagerService() {
        registerAttribute("CacheProviderClass", String.class);
        registerAttribute("CacheRegionPrefix", String.class);
        registerAttribute("CacheUseMinimalPuts", Boolean.class);
        registerAttribute("CacheUseQueryCache", Boolean.class);
        registerAttribute("DataSource", String.class);
        registerAttribute("DefaultSchema", String.class);
        registerAttribute("Dialect", String.class);
        registerAttribute("GenerateStatistics", Boolean.class);
        registerAttribute("Hbm2ddlAuto", Boolean.class);
        registerAttribute("JdbcBatchSize", Integer.class);
        registerAttribute("JdbcBatchVersionedData", Boolean.class);
        registerAttribute("JdbcFetchSize", Integer.class);
        registerAttribute("JdbcUseGetGeneratedKeys", Boolean.class);
        registerAttribute("JdbcUseScrollableResultSet", Boolean.class);
        registerAttribute("JdbcUseStreamsForBinary", Boolean.class);
        registerAttribute("MaxFetchDepth", Integer.class);
        registerAttribute("Password", String.class);
        registerAttribute("QuerySubstitutions", String.class);
        registerAttribute("ShowSQL", Boolean.class);
        registerAttribute("UseSqlComments", Boolean.class);
        registerAttribute("Username", String.class);
    }

    @Attribute(readOnly = true)
    public String getService() {
        return service;
    }

    @KeyProperty
    public void setService(String service) {
        this.service = service;
    }

    @Attribute
    public String getJndiName() {
        return jndiName;
    }

    public void setJndiName(String jndiName) {
        this.jndiName = jndiName;
    }

    @Attribute
    public boolean isStarted() {
        return started;
    }

    protected Map<String, String> remappedAttributes() {
        Map<String, String> names = new HashMap<String, String>();

        names.put("CacheProviderClass", "hibernate.cache.provider_class");
        names.put("CacheRegionPrefix", "hibernate.cache.region_prefix");
        names.put("CacheUseMinimalPuts", "hibernate.cache.use_minimal_puts");
        names.put("CacheUseQueryCache", "hibernate.cache.use_query_cache");
        names.put("DataSource", "hibernate.connection.datasource");
        names.put("DefaultSchema", "hibernate.default_schema");
        names.put("Dialect", "hibernate.dialect");
        names.put("GenerateStatistics", "hibernate.generate_statistics");
        names.put("JdbcBatchSize", "hibernate.jdbc.batch_size");
        names.put("JdbcBatchVersionedData", "hibernate.jdbc.batch_versioned_data");
        names.put("JdbcFetchSize", "hibernate.jdbc.fetch_size");
        names.put("JdbcUseGetGeneratedKeys", "hibernate.jdbc.use_get_generated_keys");
        names.put("JdbcUseScrollableResultSet", "hibernate.jdbc.use_scrollable_resultset");
        names.put("JdbcUseStreamsForBinary", "hibernate.jdbc.use_streams_for_binary");
        names.put("Hbm2ddlAuto", "hibernate.hbm2ddl.auto");
        names.put("MaxFetchDepth", "hibernate.max_fetch_depth");
        names.put("Password", "hibernate.connection.password");
        names.put("QuerySubstitutions", "hibernate.query.substitutions");
        names.put("ShowSQL", "hibernate.show_sql");
        names.put("UseSQLComments", "hibernate.use_sql_comments");
        names.put("Username", "hibernate.connection.username");

        return names;
    }

    @Operation
    public void start() throws Exception {
        if (isStarted()) {
            throw new IllegalStateException(service + " already started!");
        }

        verify();

        logger = Logger.getLogger(service);

        Properties properties = attributes.toProperties();
        PropertiesHelper.remap(properties, remappedAttributes());

        logger.debug("Initializing with properties: %s", properties);
        instance = new PersistenceManager(properties);

        helper.bind(jndiName, instance);
        logger.info("Bound PersistenceManager '%s' to JNDI name '%s'", name, jndiName);

        started = true;
    }

    @Operation
    public void stop() throws Exception {
        if (!isStarted()) {
            throw new IllegalStateException(service + " already stopped!");
        }

        helper.unbind(jndiName);
        logger.info("Unbound PersistenceManager '%s' from JNDI name '%s'", name, jndiName);

        instance = null;
        logger = null;

        started = false;
    }
}
