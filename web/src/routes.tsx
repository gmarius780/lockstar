import DeviceHubIcon from '@material-ui/icons/DeviceHub'
import SettingsIcon from '@material-ui/icons/Settings'

import DevicesPage from './pages/devices'
import SettingsPage from './pages/settings'
import NotFoundPage from './pages/not-found'

interface Route {
  path: string,
  component: any,
  label?: string,
  exact?: boolean,
  sidebar?: boolean,
  divider?: boolean,
  icon?: any,
}

export default [
  {
    path: '/',
    label: 'Devices',
    divider: true,
    icon: DeviceHubIcon,
    component: DevicesPage,
    sidebar: true,
  },
  {
    path: '/settings',
    label: 'Settings',
    exact: true,
    icon: SettingsIcon,
    component: SettingsPage,
    sidebar: true,
  },
  {
    path: '/*',
    component: NotFoundPage,
  },
] as Route[]
