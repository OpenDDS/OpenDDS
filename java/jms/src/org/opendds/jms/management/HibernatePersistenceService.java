/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management;

import java.util.Properties;

import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.JndiHelper;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Constructor;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.KeyProperty;
import org.opendds.jms.management.annotation.Operation;
import org.opendds.jms.persistence.HibernatePersistenceManager;
import org.opendds.jms.persistence.PersistenceManager;

/**
 * @author  Steven Stallion
 */
@Description("OpenDDS Hibernate PersistenceManager MBean")
public class HibernatePersistenceService extends DynamicMBeanSupport implements ServiceMBean {
    private Logger logger;

    private String service;
    private boolean started;
    private String jndiName;
    private PersistenceManager instance;

    private JndiHelper helper = new JndiHelper();

    @Constructor
    public HibernatePersistenceService() {
        registerAttribute("CacheProviderClass", "hibernate.cache.provider_class", String.class);
        registerAttribute("CacheRegionPrefix", "hibernate.cache.region_prefix", String.class);
        registerAttribute("CacheUseMinimalPuts", "hibernate.cache.use_minimal_puts", Boolean.class);
        registerAttribute("CacheUseQueryCache", "hibernate.cache.use_query_cache", Boolean.class);
        registerAttribute("DataSource", "hibernate.connection.datasource", String.class);
        registerAttribute("DefaultSchema", "hibernate.default_schema", String.class);
        registerAttribute("Dialect", "hibernate.dialect", String.class);
        registerAttribute("GenerateStatistics", "hibernate.generate_statistics", Boolean.class);
        registerAttribute("Hbm2ddlAuto", "hibernate.hbm2ddl.auto", String.class);
        registerAttribute("JdbcBatchSize", "hibernate.jdbc.batch_size", Integer.class);
        registerAttribute("JdbcBatchVersionedData", "hibernate.jdbc.batch_versioned_data", Boolean.class);
        registerAttribute("JdbcFetchSize", "hibernate.jdbc.fetch_size", Integer.class);
        registerAttribute("JdbcUseGetGeneratedKeys", "hibernate.jdbc.use_get_generated_keys", Boolean.class);
        registerAttribute("JdbcUseScrollableResultSet", "hibernate.jdbc.use_scrollable_resultset", Boolean.class);
        registerAttribute("JdbcUseStreamsForBinary", "hibernate.jdbc.use_streams_for_binary", Boolean.class);
        registerAttribute("MaxFetchDepth", "hibernate.max_fetch_depth", Integer.class);
        registerAttribute("Password", "hibernate.connection.password", String.class);
        registerAttribute("QuerySubstitutions", "hibernate.query.substitutions", String.class);
        registerAttribute("ShowSql", "hibernate.show_sql", Boolean.class);
        registerAttribute("UseSqlComments", "hibernate.use_sql_comments", Boolean.class);
        registerAttribute("Username", "hibernate.connection.username", String.class);
    }

    @Attribute(readOnly = true)
    public String getService() {
        return service;
    }

    @KeyProperty
    public void setService(String service) {
        this.service = service;
    }

    @Attribute(required = true)
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

    @Operation
    public void start() throws Exception {
        if (isStarted()) {
            throw new IllegalStateException(name + " is already started!");
        }

        verify();

        logger = Logger.getLogger(service);

        Properties properties = attributes.toProperties();

        if (logger.isDebugEnabled()) {
            logger.debug("Initializing with %s", Strings.asIdentity(properties));
        }
        instance = new HibernatePersistenceManager(properties);

        helper.bind(jndiName, instance);
        logger.info("Bound PersistenceManager '%s' to JNDI name '%s'", name, jndiName);

        started = true;
    }

    @Operation
    public void stop() throws Exception {
        if (!isStarted()) {
            throw new IllegalStateException(name + " is already stopped!");
        }

        helper.unbind(jndiName);
        logger.info("Unbound PersistenceManager '%s' from JNDI name '%s'", name, jndiName);

        instance = null;
        logger = null;

        started = false;
    }
}
