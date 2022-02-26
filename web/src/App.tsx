import React, { Suspense } from 'react'
import { HashRouter as Router, Route, Switch } from 'react-router-dom'

import routes from './routes'
import Layout from './components/layout'

export default () => (
  <Router basename={process.env.PUBLIC_URL}>
    <Suspense fallback={<div>Loading...</div>}>
      <Layout>
        <Switch>
          {routes.map((route, index) => (
            <Route
              key={index}
              path={route.path}
              exact={route.exact}
              component={route.component}
            />
          ))}
        </Switch>
      </Layout>
    </Suspense>
  </Router>
)
