import React from 'react'
import { Route, Redirect, Switch, useRouteMatch } from 'react-router-dom'

import List from './list'
import Add from './add'
import Edit from './edit'
import View from './view'
import useLocalStorage from '../../hooks/useLocalStorage'
import NotFoundPage from '../not-found'

export default () => {
  const { path } = useRouteMatch()
  const [serverBaseUrl] = useLocalStorage('serverBaseUrl')

  if (!serverBaseUrl) {
    return <Redirect to={'/welcome'} />
  }

  return (
    <>
      <Switch>
        <Route exact path={path} component={List} />
        <Route exact path={`${path}/add`} component={Add} />
        <Route exact path={`${path}/view/:deviceId(\\d+)`} component={View} />
        <Route exact path={`${path}/edit/:deviceId(\\d+)`} component={Edit} />
        <Route path={`${path}/*`} component={NotFoundPage} />
      </Switch>
    </>
  )
}
