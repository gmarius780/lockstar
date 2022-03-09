import DeviceHubIcon from '@material-ui/icons/DeviceHub'


import DevicesPage from './pages/devices'

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
    path: '/*',
    component: NotFoundPage,
  },
] as Route[]
